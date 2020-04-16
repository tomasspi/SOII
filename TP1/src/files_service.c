/**
 * @file files_service.c
 * @brief
 * Proceso encargado de proveer las funcionalidades relacionadas a las
 * imágenes disponibles para decargar y la transferencia de la imagen
 * seleccionada.
 * 
 * @author Tomás Santiago Piñero
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "messages.h"
#include "sockets.h"

int main(void)
{
  printf("Files service started.\n");

  struct msg buf;
  int msqid;
  key_t key;

  if ((key = ftok(QU_PATH, 5)) == -1)
  {
    perror("ftok");
    exit(EXIT_FAILURE);
  }

  if ((msqid = msgget(key, 0666)) == -1)
  {
    perror("msgget");
    exit(EXIT_FAILURE);
  }

  buf.mtype = TO_PRIM;

  while (1)
  {
    msgrcv(msqid, &buf, sizeof(buf.msg), TO_FILE, 0);

    if(!strcmp(buf.msg, "exit")) exit(EXIT_SUCCESS);


  }

  return EXIT_SUCCESS;
}
