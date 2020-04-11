#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "sockets.h"

/**
 * Escribe en el socket deseado un mensaje.
 * @param  sockfd Socket en el que se desea escribir el mensaje.
 * @param  cmd    Comando enviado por el usuario.
 * @return        Si el write fue exitoso.
 */
ssize_t send_cmd(int sockfd, void *cmd)
{
  char buf[MAX];
  strcpy(buf,(char*) cmd);
  ssize_t s;

  s = write(sockfd, buf, strlen(buf));

  return s;
}

/**
 * Lee lo que se encuentra en el socket.
 * @param  sockfd Socket del que se desea leer el mensaje.
 * @param  cmd    Comando enviado por el usuario.
 * @return        Si el read fue exitoso.
 */
ssize_t recv_cmd(int sockfd, void *cmd)
{
  char buf[MAX];
  ssize_t r;
  memset(buf, '\0', MAX);
  r = recv(sockfd, buf, MAX-1, 0);
  strcpy((char *) cmd, buf);
  return r;
}

/**
 * Función encargada de crear el socket del servidor en el puerto pedido.
 * @param  port Puerto al que se desea conectar.
 * @return      File descriptor del socket creado.
 */
int create_svsocket(uint16_t port)
{
  int sockfd;
  struct sockaddr_in sv_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if(sockfd == -1)
  {
    perror("create");
    exit(EXIT_FAILURE);
  }

  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

  memset((char *) &sv_addr, 0, sizeof(sv_addr));

  sv_addr.sin_family = AF_INET;
  sv_addr.sin_addr.s_addr = INADDR_ANY;
  sv_addr.sin_port = htons(port);

  if(bind(sockfd, (struct sockaddr *) &sv_addr, sizeof(sv_addr)) == -1)
  {
    perror("bind");
    exit(EXIT_FAILURE);
  }
  return sockfd;
}

/**
 * Función encargada de conectar al cliente en el puerto del servidor.
 * @param  port Puerto al que se desea conectar.
 * @return      File descriptor del socket conectado.
 */
int create_clsocket(uint16_t port)
{
  int sockfd;
  struct sockaddr_in sv_addr;
  struct hostent *server;

  sockfd = socket( AF_INET, SOCK_STREAM, 0 );
  if ( sockfd == -1 )
  {
    perror("create");
    exit(EXIT_FAILURE);
  }

  server = gethostbyname("localhost");

  if (server == NULL)
  {
    perror("get server");
    exit(EXIT_FAILURE);
  }

  memset((char *) &sv_addr, '0', sizeof(sv_addr));
  sv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,(char *)&sv_addr.sin_addr.s_addr,(size_t) server->h_length);
  sv_addr.sin_port = htons(port);

  if(connect(sockfd, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) == -1)
  {
    perror("connect");
    exit(EXIT_FAILURE);
  }

  return sockfd;
}
