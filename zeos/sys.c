/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

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
	int PID = -1;

	// creates the child process

	return PID;
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
	* Dudas Alex
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
