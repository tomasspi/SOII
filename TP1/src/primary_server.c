#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "messages.h"
#include "sockets.h"

#define USER "user"
#define FILE "file"

/**< Comandos soportados. */
char *builtins[] =
{
  "user ls",
  "user passwd",
  "file ls",
  "file down",
  "exit"
};

/**
 * Función encargada de tomar las credenciales (usuario y contraseña) para el
 * login. Estas credenciales se guardan en la estructura 'log_msg' y se la
 * envía al proceso encargado del servicio de autenticación ('auth_service').
 * @param struct log_msg  Mensaje con las credenciales del usuario.
 */
void login(struct log_msg *log, char buf[MAX]);

int main(void)
{
  int sockfd = create_svsocket(PORT_PS);
  int newfd;

  ssize_t rw;
  char msg_buf[MAX];

  struct log_msg log;
  struct msg buf;
  int msqid;
  key_t key;

  if ((key = ftok("../src/primary_server.c", 5)) == -1)
  {
    perror("ftok");
    exit(EXIT_FAILURE);
  }

  if ((msqid = msgget(key, 0666 | IPC_CREAT)) == -1)
  {
    perror("msgget");
    exit(EXIT_FAILURE);
  }

  log.mtype = TO_AUTH;

  struct sockaddr_in cl_addr;
  uint cl_len;

  listen(sockfd, 1);
  cl_len = sizeof(cl_addr);

  newfd = accept(sockfd, (struct sockaddr *) &cl_addr, &cl_len);

  if(newfd == -1)
  {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  printf("Conexión aceptada.\n");

  close(sockfd);

  int loginf = 1;

  int auth_id, files_id, auth_state, files_state;

  if((auth_id = fork()) == -1)
  {
    perror("fork auth");
    exit(EXIT_FAILURE);
  }

  char *arga[] = {"./auth", NULL};
  char *argf[] = {"./files", NULL};

  if(auth_id == 0) execv(arga[0], arga);
  else
  {
    if((files_id =  fork()) == -1)
    {
      perror("fork files");
      exit(EXIT_FAILURE);
    }

    if(files_id == 0) execv(argf[0], argf);
    else
    {
      waitpid(auth_id, &auth_state, WNOHANG);
      waitpid(files_id, &files_state, WNOHANG);

      while(1)
      {
        /**< Lectura */
        rw = recv_cmd(newfd,msg_buf);

        if(rw == -1)
        {
          perror("read");
          exit(EXIT_FAILURE);
        }
        /**< Fin Lectura */

        if(loginf)
        {
          /**< Check login. */
          login(&log, msg_buf);

          msgsnd(msqid, &log, sizeof(log.login), 0);

          msgrcv(msqid, &buf, sizeof(buf.msg), TO_PRIM, 0);

          if(!strcmp(buf.msg,"A")) loginf = 0;
          /**<End login. */
        }

        /**< Escritura */
        rw = send_cmd(newfd, buf.msg);

        if(rw == -1)
        {
          perror("write");
          exit(EXIT_FAILURE);
        }
        msg_buf[strlen(msg_buf)-1] = '\0';
        /**< Escritura */

        if(!strcmp("exit",msg_buf))
        {
          close(newfd);
          exit(EXIT_SUCCESS);
        }
      }

      if (msgctl(msqid, IPC_RMID, NULL) == -1) /* Destruye la cola */
      {
        perror("msgctl");
        exit(1);
      }
    }
  }

  return EXIT_SUCCESS;
}

/**
 * Función encargada de tomar las credenciales (usuario y contraseña) para el
 * login. Estas credenciales se guardan en la estructura 'log_msg' con el
 * fin de enviar las credenciales a 'auth_service'.
 * @param struct log_msg  Mensaje con las credenciales del usuario.
 */
void login(struct log_msg *log, char buf[MAX])
{
  char *tok;
  tok = strtok(buf,",");

  strcpy(log->login.username,tok);

  tok = strtok(NULL,"");

  strcpy(log->login.password,tok);
}
