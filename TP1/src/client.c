
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

void ask_login(char buf[STR_LEN]);
void show_prompt(int32_t sockfd, ssize_t rw, char buffer[STR_LEN]);

int32_t sockfd;

/**
 * @brief
 * Handler para detectar la señal 'Ctrl+C' y enviar el comando 'exit'.
 * @param fd File descriptor del socket a cerrar.
 */
void sig_handler()
{
  send_cmd(sockfd,"exit");
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

  int32_t tries = 3;

  char buffer[STR_LEN];

	sockfd = create_clsocket(PORT_PS);
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

    if(strchr(buffer,'/') != NULL)
    {
      printf("\nLast login: %s\n", buffer);
      show_prompt(sockfd, rw, buffer);
    } else
    {
      printf("%s\n", buffer);
      tries--;
    }
  } while(tries > 0);

  return EXIT_SUCCESS;
}

/**
 * @brief
 * Función encargada de tomar las credenciales (usuario y contraseña) para el
 * login. Estas credenciales se guardan en la estructura 'log_msg' con el
 * fin de enviar las credenciales a 'auth_service'.
 * @param struct log_msg  Mensaje con las credenciales del usuario.
 */
void ask_login(char buf[STR_LEN])
{
  printf("Ingrese usuario: ");
  if(fgets(buf, STR_LEN, stdin) == NULL) perror("fgets login");
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

/**
 * @brief
 * Función encargada de mostrar el 'prompt' al usuario en caso de ser validadas
 * las credenciales enviadas.
 * @param sockfd File descriptor del socket al servidor.
 * @param rw     Resultado de la operación de Lectura/escritura.
 * @param buffer Mensaje enviado/recibido.
 */
void show_prompt(int32_t sockfd, ssize_t rw, char buffer[STR_LEN])
{
  prompt:

  while(1)
  {
		printf("#> ");
		memset(buffer, '\0', STR_LEN);
		if(fgets(buffer, STR_LEN-1, stdin) == NULL) perror("fgets prompt");
    buffer[strlen(buffer)-1] = '\0';

    if(buffer[0] == '\0') goto prompt;
    
		rw = send_cmd(sockfd, buffer);

    if(rw == -1)
    {
      perror("write");
      exit(EXIT_FAILURE);
    }

		// Verificando si se escribió: exit
    if(!strcmp(buffer, "exit")) exit(EXIT_SUCCESS);


    rw = recv_cmd(sockfd, buffer);

    printf("%s\n", buffer);

    if(rw == -1)
    {
      perror("read");
      exit(EXIT_FAILURE);
    }
	}
}
