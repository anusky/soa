#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


// Create a socket and initialize it to be able to accept 
// connections.
// It returns the virtual device associated to the socket that should be used 
// in the accept system call, to get the virtual device associated to 
// the connection
//

int createServerSocket (int port) {
  //int socket(int domain, int type, int protocol);
  /**
   *  domain    :   AF_INET | PF_INET | PF_UNIX
   *  types     :   SOCK_STREAM (REMOTO) | SOCK_DGRAM (LOCAL)
   *  protocol  :   Dejar con 0 y el sistema elige el apropiado
   *  
   *  returns   :   Canal asociado si OK ; -1 si KO
   */

  //int bind();
  /**
   *  domain    :   AF_INET | PF_INET | PF_UNIX
   *  types     :   SOCK_STREAM (REMOTO) | SOCK_DGRAM (LOCAL)
   *  protocol  :   Dejar con 0 y el sistema elige el apropiado
   *  
   *  returns   :   Canal asociado si OK ; -1 si KO
   */

  //int listen(int canal, int backlog);
  /**
   *  canal       :   el que he creado con socket()
   *  backlog     :   #peticiones pendientes que puede tener un servidor. 
   *                  Si se supera el cliente recibe error en la peticion de conexion
   *  
   *  returns     :   0 - OK | -1 KO
   */
  
  struct sockaddr_in socket_address;
  int socket_fd;

  socket_fd = socket(AF_INET, SOCK_STREAM, 0); //CANAL
  
  if(socket_fd != -1)  {
    socket_address.sin_family_t = PF_INET;
    socket_address.sin_addr = INADDR_ANY; //representa IP de la maquina donde ejecutamos el codigo
    socket_address.sin_port = port; 


    int socket_bind = bind(socket_fd, &(socket_address), sizeof(socket_address));

    if(socket_bind < 0) {
      //errno ¿?
      return -1;
    } else {
      return listen(socket_fd, 3);
    }
    return -1;  
  }
  return socket_fd;

}


// Returns the file descriptor associated to the connection.
// accept system call will fill the socketAddr parameter
// with the address of the socket for the client which is requesting the
// connection, and the addrSize parameter with the size of that address.

int
acceptNewConnections (int socket_fd)
{
  //int accept(int canal, struct sockaddr *direccion, socklen_t *tam_direccion);
  /**
   *  canal    :   canal que recibo por parametro
   *  direccion     :   obiene la direccion del cliente que solicita la sireccion
   *  tam_direccion  :   contiene el tamaño que ocupa la direccion
   *  
   *  returns   :   Canal para usar en la transmision si OK ; -1 si KO
   */
  struct sockaddr_in serv_addr;
  int canal = accept(socket_fd, &(serv_addr), sizeof(serv_addr));
}

// Returns the socket virtual device that the client should use to access 
// the socket, if the connection is successfully established 
// and -1 in case of error
//
// Connect system call requires the address of the 
// server socket to request the connection and the size of that address.
//

int
clientConnection (char *host_name, int port)
{

  struct sockaddr_in serv_addr;
  struct hostent * hent;
  int socket_fd;
  int ret;

  //creates the virtual device for accessing the socket
  socket_fd = socket (AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0)
    return socket_fd;

  memset((char *) &serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  hent = gethostbyname(host_name);
  if (hent == NULL) {
        close (socket_fd);
	return -1;

  }
  memcpy((char *)&serv_addr.sin_addr.s_addr, (char *) hent->h_addr, hent->h_length);
  serv_addr.sin_port = htons(port);
  serv_addr.sin_family = PF_INET; 

  ret = connect (socket_fd, (struct sockaddr *) &serv_addr, sizeof (serv_addr));
  if (ret < 0)
  {
	  close (socket_fd);
	  return (ret);
  } 

  return socket_fd;

}


int deleteSocket (int socket_fd)
{
  close (socket_fd);
}

