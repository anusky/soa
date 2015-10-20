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

}


int sys_write(int fd, char * buffer, int size) {
	int ret;
	if ( (ret = check_fd(fd, ESCRIPTURA)) ) {
		return ret;
	}
	if (buffer == NULL) {
		return -1; //Dudas Alex
	}
	if (size < 0) {
		return -1;
	}

	/*
	* Dudas Alex (Done) @TODO
	* Comentarios: Estando en modo sistema no debería trabajar con la pila del usuario
	* De modo que la copiamos desde usuario a sistema y luego la devolvemos
	* copy_from_user --  Copy a block of data from user space.
	* unsigned long copy_from_user (void * to, const void __user * from, unsigned long n);
	*
	* copy_to_user --  Copy a block of data into user space.
	* unsigned long copy_to_user (void __user * to, const void * from, unsigned long n);
	*/
	return ret;
}

extern int zeos_ticks;

int sys_gettime() {
	return zeos_ticks;
}


void sys_exit()
{
}
