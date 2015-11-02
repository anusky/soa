/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1
#define TAM_BUFFER 512

extern int zeos_ticks;
extern int remaining_quantum;


int check_fd(int fd, int permissions)
{
	if (fd != 1) return -9; /*EBADF*/
	if (permissions != ESCRIPTURA) return -13; /*EACCES*/
	return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int ret_from_fork()
{
  return 0;
}

int sys_fork()
{
	struct list_head *new = NULL;
	union task_union *child;

	if(list_empty( &freequeue )) return -ENOMEM;

	new = list_first( &freequeue );
  	list_del(new);

  	child = (union task_union*) list_head_to_task_struct(new);
  	copy_data(current(), child, sizeof(union task_union));
  	allocate_DIR( (struct task_struct*)child);

  	/*PAG_LOG_INIT_DATA  es la pagina logica inicial a partir de la cual residen los datos*/
  	/*reservamos memoria para guardar data+stack*/
  	int pag, physical_page, aux;
  	page_table_entry *process_pt = get_PT(&child->task); //me da la TP del hijo (task)
  	for(pag = 0; pag < NUM_PAG_DATA; pag++) {
  		physical_page = alloc_frame();
  		if(physical_page == -1) {
  			//error no tengo paginas libres, debo borrar entradas temporales en la tabla de paginas
  			//libero aquellas paginas que he llegado a reservar porque no podre realizar todo el proceso.
  			for(aux = 0; aux < pag; ++aux) {
  				free_frame(get_frame(process_pt, PAG_LOG_INIT_DATA+pag, physical_page));
  				del_ss_pag(process_pt, PAG_LOG_INIT_DATA+aux);
  			}
  			/*libero la task_struct de la cola de libres */
  			list_add_tail(new, &freequeue);

  			return -EAGAIN;
  		}
  		else {
  			set_ss_pag(process_pt, PAG_LOG_INIT_DATA+pag, physical_page);
  		}
  	}

  	/*copiamos el contexto del padre al hijo + su codigo*/
  	page_table_entry *parent_pt = get_PT(current());
  	for(pag = 0; pag < NUM_PAG_KERNEL; pag++) {
  		set_ss_pag(parent_pt, pag, get_frame(parent_pt, pag));
  	}
  	for(pag = 0; pag < NUM_PAG_CODE; pag++) {
	    set_ss_pag(process_pt, PAG_LOG_INIT_CODE+pag, get_frame(parent_pt, PAG_LOG_INIT_CODE+pag));
  	}

  	/* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
	for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
	{
	/* Map one child page to parent's address space. */
		set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
		copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
		del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
	}

	//flush tlb
	set_cr3( get_DIR( current() ) );
	child->task.PID = global_PID++;
	child->task.state = ST_READY;

	int register_ebp;		/* frame pointer */
	/* Map Parent's ebp to child's stack */
	__asm__ __volatile__ (
		"movl %%ebp, %0\n\t"
	    : "=g" (register_ebp)
	    : );
	register_ebp=(register_ebp - (int)current()) + (int)(child);

	child->task.register_esp=register_ebp + sizeof(DWord);
	  
	DWord temp_ebp=*(DWord*)register_ebp;
	/* Prepare child stack for context switch */
	child->task.register_esp-=sizeof(DWord);
	*(DWord*)(child->task.register_esp)=(DWord)&ret_from_fork;
	child->task.register_esp-=sizeof(DWord);
	*(DWord*)(child->task.register_esp)=temp_ebp;

	child->task.state=ST_READY;
  	list_add_tail(&(child->task.list), &readyqueue);
  
  	return child->task.PID;
}


int sys_write(int fd, char * buffer, int size) {
	/**
	 *	fd - fileDescriptor. Aqui siempre sera 1
	 * 	buffer - pointer to the bytes
	 *	size - number of bytes to write
	 *	Retorna -> num negativo si error (especifica el tipo de error) ; numero de bytes escritos si OK
	 *	Creo un buffer para no perder info en caso que sea mayor al tamaño que puedo imprimir
	 *	luego lo recorro, recogiendo de lapila del usuario lo necesario, clonandola en la pila de sistema.
	 */
	
	
	int ret;
	if ( (ret = check_fd(fd, ESCRIPTURA)) ) {
		return ret;
	}
	if (size < 0) {
		return -EINVAL;
	}
	if(!access_ok(VERIFY_WRITE, buffer, size)) {
		return -EFAULT;
	}

	int bytes_left = size;
	char localbuffer[TAM_BUFFER];
	while(bytes_left > TAM_BUFFER)  {
		/* los bytes que quedan son mayores que mi buffer, trabajo con buffer. Copio la pila de usuario */
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		/* me traigo el buffer del usuario a éste, en local, que es el de sistema */
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left -= ret;
		/* resto de los bytes que quedan, el tamaño de bytes que he escrito */
		buffer += ret;
		/* 	buffer es un puntero, lo avanzo con la cantidad de bytes que he usado, 
		*	para posicionar el puntero en el siguiente dato que quiero escribir.
		*/	
	}

	if(bytes_left > 0) {
		/* me quedan bytes que escribir pero no llegan a ocupar lo suficiente para usar el buffer*/
		copy_from_user(buffer, localbuffer, bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left -= ret;
	}

	/*
	* Dudas Alex (Done)
	* Comentarios: Estando en modo sistema no debería trabajar con la pila del usuario
	* De modo que la copiamos desde usuario a sistema y luego la devolvemos
	* copy_from_user --  Copy a block of data from user space.
	* unsigned long copy_from_user (void * to, const void __user * from, unsigned long n);
	*
	* copy_to_user --  Copy a block of data into user space.
	* unsigned long copy_to_user (void __user * to, const void * from, unsigned long n);
	*/

	/* retorno la cantidad de bytes que he escrito */
	return (size - bytes_left);
}


int sys_gettime() {
	return zeos_ticks;
}


void sys_exit()
{
  int i;

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA_P0+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA_P0+i);
  }
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID=-1;
  
  /* Restarts execution of the next process */
  sched_next();
}

int sys_get_stats(int pid, struct stats *st)
{
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}
