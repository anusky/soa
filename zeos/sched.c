/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS + 2]
  __attribute__((__section__(".data.task")));

union task_union * task = & protected_tasks[1]; /* == union task_union task[NR_TASKS] */

#if 0
struct task_struct * list_head_to_task_struct(struct list_head * l) 
{
    return list_entry(l, struct task_struct, list);
}
#endif

extern struct list_head blocked;

/* task struct libres */
struct list_head freequeue;
/* task struct listas para ejecutarse */
struct list_head readyqueue;

struct task_struct * idle_task;

/*variables para el scheduling*/
#define DEFAULT_QUANTUM 10
int remaining_quantum=0;


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR(struct task_struct * t) {
    return t-> dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT(struct task_struct * t) {
    return (page_table_entry * )(((unsigned int)(t-> dir_pages_baseAddr-> bits.pbase_addr)) << 12);
}


int allocate_DIR(struct task_struct * t) {
    int pos;


    pos = ((int) t - (int) task) / sizeof(union task_union);

    t-> dir_pages_baseAddr = (page_table_entry * ) & dir_pages[pos];

    return 1;
}

void cpu_idle(void) {
    __asm__ __volatile__("sti": : : "memory");

    while (1) 
    {
    ;
    }
}

void init_freequeue() {
    int i;
    INIT_LIST_HEAD( &freequeue);

    /*
     * task apunta a un array de tipo task_union{task, stack[]}
     * y task.task es de tipo task_struct{pid, list}
     */

    for (i = 0; i < NR_TASKS; i++) {
        task[i].task.PID = -1; //su pid es -1 puesto que estamos inicializando y no tiene proceso asociado.
        list_add_tail( &(task[i].task.list), &freequeue);
    }
}

void init_readyqueue() {
    INIT_LIST_HEAD( & readyqueue);
}

void init_idle(void) {
	/*
	* tengo list_first(head)
	* Le tengo que pasar el &freequeue
	*/
	struct list_head * available = list_first( &freequeue );
  list_del(available);
  struct task_struct *task_available =list_head_to_task_struct( available );
  union task_union *new = (union task_union*)task_available;

  task_available->PID = 0;
	allocate_DIR( task_available );

  new->stack[KERNEL_STACK_SIZE - 2] = 0; /*registro ebp a 0 - Indico inicio del codigo*/
  new->stack[KERNEL_STACK_SIZE - 1] = (unsigned long)&cpu_idle; /*Aqui me guardo la direccion de retorno a cpu_idle*/
	
  task_available->register_esp = (int) &(new->stack[KERNEL_STACK_SIZE-2]);


	idle_task = task_available;
}

void init_task1(void) {
  struct list_head *available = list_first( &freequeue );
  list_del(available);
  struct task_struct *task_available =list_head_to_task_struct( available );
  union task_union *new = (union task_union*)task_available;

  task_available->PID = 1;
  allocate_DIR ( task_available );

  set_user_pages( task_available );

  tss->esp0 = (DWord) &(new->stack[KERNEL_STACK_SIZE]);
  set_cr3( task_available->dir_pages_baseAddr);

}


void init_sched() {
    init_freequeue();
    init_readyqueue();

}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

void inner_task_switch(union task_union *new) {
  page_table_entry *new_direction = get_DIR(&new->task);

  tss->esp0 = (DWord) &(new->stack[KERNEL_STACK_SIZE]); //actualizo el TSS para que apunte a la nueva pila
  

  set_cr3( new_direction ); //hacemos flush de la TLB actualizando el cr3 y creando un nuevo espacio de direcciones

    /*nos guardamos el valor de ebp y lo guardamos en el registro_esp del actual PCB*/
  __asm__ __volatile__ (
    "movl %%ebp, %0\n\t"
    : "=g" (current()->register_esp)
    :);

    /*restauramos el nuevo valor de esp y lo guardamos en el esp del nuevo PCB*/
  __asm__ __volatile__ (
    "movl %0, %%ebp\n\t"
    :
    :"g" (new->task.register_esp));

  /* restauramos el valor de ebp de la pila */
  __asm__ __volatile__ (
    "popl %%ebp\n\t");

  /* usamos la instruccion ret para volver a la rutina que nos llamo */
  __asm__ __volatile__ (
    "ret\n\t");

}

void task_switch(union task_union *new) {
  __asm__ __volatile__ (
    "pushl %esi\n\t"
    "pushl %edi\n\t"
    "pushl %ebx\n\t" );

  inner_task_switch(new);

  __asm__ __volatile__ (
    "pushl %esi\n\t"
    "pushl %edi\n\t"
    "pushl %ebx\n\t" );
  
}

int get_quantum(struct task_struct *t) {
  return t->total_quantum;
}

void set_quantum(struct task_struct *t, int new_quantum) {
  t->total_quantum = new_quantum;
}

void update_sched_data_rr() {
  remaining_quantum--;
}

int needs_sched_rr() {
  if( remaining_quantum == 0 && (!list_empty(&readyqueue)) ) {
    return 1;
  }
  if( remaining_quantum == 0 ) {
    remaining_quantum = get_quantum(current());
  }
  return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue) {
  if(t->state != ST_RUN) {
    list_del(&(t->list));
  }
  if(dst_queue != NULL) {
    list_add_tail(&(t->list), dst_queue);
    if (dst_queue!=&readyqueue) t->state=ST_BLOCKED;
    else
    {
      update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
      t->state=ST_READY;
    }
  }
  else t->state=ST_RUN;
}

void sched_next_rr() {
  struct list_head *e;
  struct task_struct *t;

  e=list_first(&readyqueue);

  if (e)
  {
    list_del(e);

    t=list_head_to_task_struct(e);
  }
  else
    t=idle_task;

  t->state=ST_RUN;
  remaining_quantum=get_quantum(t);

  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
  update_stats(&(t->p_stats.ready_ticks), &(t->p_stats.elapsed_total_ticks));
  t->p_stats.total_trans++;

  task_switch((union task_union*)t);
}

void schedule() {
  update_sched_data();
  if (needs_sched())
  {
    update_process_state(current(), &readyqueue);
    sched_next();
  }
}