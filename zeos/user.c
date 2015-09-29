
#include <stdio.h>
#include <libc.h>


char buff[24];

int pid;

int add(int par1, int par2) {
	int ret;
	//=a le indicamos que quiero usar el registro a y que guardare alli 
	//la suma de par1 y par2 
	//"b"(par2) le indico que quiero usar el registro b, y que cargo en él, el contenido de par2
	//"a"(par1) --> lo mismo que con b pero con el registro a y par1
	__asm__ __volatile__(
		"addl %%ebx, %%eax"
		:"=a"(ret) /* output */
		:"a"(par1), "b"(par2)  /* input */
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
/*	long count,acum;
	count = 75;
	acum = 0;
	acum = outer(count);
*/
	
	//dd(1, 2); //res 3
	
	
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

    
  	while(1);
	return 0;
}

