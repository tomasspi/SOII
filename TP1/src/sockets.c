/**
 * @file sockets.c
 * @brief
 * Header con las utilizades para la utilización de los sockets.
 *
 * @author Tomás Santiago Piñero
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "sockets.h"
#include "messages.h"
#include "utilities.h"

/**
 * @brief
 * Escribe en el socket deseado un mensaje.
 * @param  sockfd Socket en el que se desea escribir el mensaje.
 * @param  cmd    Comando enviado por el usuario.
 * @return        Si el write fue exitoso.
 */
ssize_t send_cmd(int sockfd, void *cmd)
{
  char buf[STR_LEN];
  strcpy(buf,(char*) cmd);
  ssize_t s;

  s = send(sockfd, buf, strlen(buf), 0);

  return s;
}

/**
 * @brief
 * Lee lo que se encuentra en el socket.
 * @param  sockfd Socket del que se desea leer el mensaje.
 * @param  cmd    Comando enviado por el usuario.
 * @return        Si el read fue exitoso.
 */
ssize_t recv_cmd(int sockfd, void *cmd)
{
  char buf[STR_LEN];
  ssize_t r;
  memset(buf, '\0', STR_LEN);
  r = recv(sockfd, buf, STR_LEN-1, 0);
  strcpy((char *) cmd, buf);
  return r;
}

/**
 * @brief
 * Función encargada de crear el socket del servidor en el puerto pedido.
 * @param  port Puerto al que se desea conectar.
 * @return      File descriptor del socket creado.
 */
int create_svsocket(char *ip, uint16_t port)
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
  sv_addr.sin_addr.s_addr = inet_addr(ip);
  sv_addr.sin_port = htons(port);

  int b = bind(sockfd, (struct sockaddr *) &sv_addr, sizeof(sv_addr));

  if(b == -1)
    {
      perror("bind");
      exit(EXIT_FAILURE);
    }
  return sockfd;
}

/**
 * @brief
 * Función encargada de conectar al cliente en el puerto del servidor.
 * @param  port Puerto al que se desea conectar.
 * @return      File descriptor del socket conectado.
 */
int create_clsocket(char *ip, uint16_t port)
{
  int sockfd;
  struct sockaddr_in sv_addr;

  sockfd = socket( AF_INET, SOCK_STREAM, 0 );

  if (sockfd == -1)
    {
      perror("create");
      exit(EXIT_FAILURE);
    }

  memset((char *) &sv_addr, '0', sizeof(sv_addr));

  sv_addr.sin_family = AF_INET;
  sv_addr.sin_addr.s_addr = inet_addr(ip);
  sv_addr.sin_port = htons(port);

  int c = connect(sockfd, (struct sockaddr *)&sv_addr, sizeof(sv_addr));

  if(c == -1)
  {
    perror("connect");
    exit(EXIT_FAILURE);
  }

  return sockfd;
}
