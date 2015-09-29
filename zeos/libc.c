/*
 * libc.c
 */

#include <libc.h>

#include <types.h>

int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;

  if (a == 0) { b[0] = '0'; b[1] = 0; return ;}

  i = 0;
  while (a > 0)
  {
    b[i] = (a % 10) + '0';
    a = a / 10;
    i++;
  }

  for (i1 = 0; i1 < i / 2; i1++)
  {
    c = b[i1];
    b[i1] = b[i - i1 - 1];
    b[i - i1 - 1] = c;
  }
  b[i] = 0;
}

int strlen(char *a)
{
  int i;

  i = 0;

  while (a[i] != 0) i++;

  return i;
}


void perror() {
  char buffer[256];

  itoa(errno, buffer);  //convierto el valor del error en un string con el mensaje de error

  write(1, buffer, strlen(buffer));  //1 por el enunciado - StdOut - Salida default - consola
}

int write(int fd, char * buffer, int size) {
  int res; //uso una var local para que no solape el valor de eax despues
  __asm__ __volatile__(
    "int $0x80\n\t" //enter kernel mode - modo sys
    : "=a" (res)
    : "a"(4), "b" (fd), "c"(buffer), "d"(size));
  if (res < 0) {
    errno = -res; //hay error, modifico errno cn el valor del error en absoluto
    return -1; //retorno al usuario -1
  }
  errno = 0;
  return res;
}

int gettime()
{
  int res;

  __asm__ __volatile__ (
    "int $0x80\n\t"
    :"=a" (res)
    :"a" (10) );
  errno = 0;
  return res;
}
