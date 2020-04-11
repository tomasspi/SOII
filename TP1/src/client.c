
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>

#include "sockets.h"

void ask_login(char buf[MAX]);

/**
 * Handler para detectar la señal 'Ctrl+C' y enviar el comando 'exit'.
 * @param fd File descriptor del socket a cerrar.
 */
void sig_handler(int fd)
{
  send_cmd(fd,"exit");
  printf("\n");
  exit(EXIT_SUCCESS);
}

int main(void)
{
  struct sigaction ctrlc;

  ctrlc.sa_handler = sig_handler;
  sigemptyset(&ctrlc.sa_mask);
  ctrlc.sa_flags = 0;

  if(sigaction(SIGINT, &ctrlc, NULL) == -1)
  {
    perror("signal");
    exit(EXIT_FAILURE);
  }

  int32_t terminar = 0;
  int32_t authorized = 0;
  int32_t tries = 3;

  char buffer[MAX];

	int32_t sockfd = create_clsocket(PORT_PS);
  ssize_t rw;

  do
  {
    ask_login(buffer);

    rw = send_cmd(sockfd,buffer);

    if(rw == -1)
    {
      perror("write");
      exit(EXIT_FAILURE);
    }

    rw = recv_cmd(sockfd, buffer);

    if(!strcmp(buffer,"A")) authorized = 1;
    else
    {
      printf("Credenciales inválidas.\n");
      tries--;
    }
  } while(!authorized && tries > 0);


  while(1)
  {
		printf( "#> ");
		memset( buffer, '\0', MAX );
		fgets( buffer, MAX-1, stdin );
    buffer[strlen(buffer)] = '\0';

    printf("Len: %lu, msg: %s\n", strlen(buffer), buffer);

		rw = send_cmd(sockfd, buffer);

    if(rw == -1)
    {
      perror("write");
      exit(EXIT_FAILURE);
    }

		// Verificando si se escribió: exit
		buffer[strlen(buffer)-1] = '\0';
		if(!strcmp("exit", buffer))
			terminar = 1;

    //memset(buffer, '\0', MAX);
		rw = recv_cmd(sockfd, buffer);

    if(rw == -1)
    {
      perror("read");
      exit(EXIT_FAILURE);
    }

		printf( "Respuesta: %s\n", buffer );
		if( terminar ) {
			printf( "Bye.\n" );
			exit(EXIT_SUCCESS);
		}
	}
}

/**
 * Función encargada de tomar las credenciales (usuario y contraseña) para el
 * login. Estas credenciales se guardan en la estructura 'log_msg' con el
 * fin de enviar las credenciales a 'auth_service'.
 * @param struct log_msg  Mensaje con las credenciales del usuario.
 */
void ask_login(char buf[MAX])
{
  printf("Ingrese usuario: ");
  fgets(buf, MAX, stdin);
  buf[strlen(buf)-1] = ',';

  printf("Ingrese contraseña: ");

  fflush(stdout);

  char c;

  struct termios auxiliar, original;
  tcgetattr(STDIN_FILENO, &auxiliar);
  original = auxiliar;
  auxiliar.c_lflag &= !ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &auxiliar);

  size_t i = strlen(buf);

  while(read(STDIN_FILENO, &c, 1) == 1 && c != '\n')
  {
    buf[i] = c;
    i++;
  }
  buf[i] = '\0';
  printf("\n");

  tcsetattr(STDIN_FILENO, TCSANOW, &original);
}
