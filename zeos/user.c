#include <libc.h>

char buff[24];

int pid;

int add(int par1, int par2) {
	/*
	*	Declaro una local para no machacar el valor de eax después
	*/
	int ret;
	__asm__ __volatile__("addl %%ebx, %%eax"
							:"=a"(ret)
							:"a"(par2), "b"(par2)
		);
	return ret;
}

long inner(long n)
{
	int i;
	long suma;
	suma = 0;
	for(i=0; i<n; i++) suma += i;
	return suma;
}

long outer(long n)
{
	int i;
	long acum;
	acum = 0;
	for(i=0;i<n; i++) acum += inner(i);
	return acum;
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
	/*long count,acum;
	count = 75;
	acum = 0;
	acum = outer(count);*/
    

    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

    int par1, par2, suma;
    par1 = 3, par2 = 6;
    suma = add(par1, par2);


  	//while(1);
	return 0;
}
