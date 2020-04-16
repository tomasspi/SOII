/**
 * @file primary_server.c
 * @brief
 * Proceso con el rol de 'middleware'. Es el encargado de establecer las
 * conexiones y utiliza los procesos 'auth' y 'fileserv' para cumplir los
 * requerimientos del sistema.
 *
 * @author Tomás Santiago Piñero
 */

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

#define USER 'u'  /**< El comando 'user'. */
#define FILE 'f'  /**< El comando 'file'. */
#define LS   "ls" /**< El comando 'ls'. */

void send_client(ssize_t rw, int newfd, struct msg *buf);

int main(void)
{
  int sockfd = create_svsocket(PORT_PS);
  int newfd;

  ssize_t rw;
  char msg_buf[STR_LEN];

  struct msg buf;
  int msqid;
  key_t key;

  if ((key = ftok(QU_PATH, 5)) == -1)
  {
    perror("ftok");
    exit(EXIT_FAILURE);
  }

  if ((msqid = msgget(key, 0666 | IPC_CREAT)) == -1)
  {
    perror("msgget");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in cl_addr;
  uint cl_len;

  printf("Esuchando en puerto %d...\n", PORT_PS);

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

  //int loginf = 1;

  int auth_id, files_id, auth_state, files_state;

  if((auth_id = fork()) == -1) //------ Auth de hijo
  {
    perror("fork auth");
    exit(EXIT_FAILURE);
  }

  char *arga[] = {"./auth", NULL};
  char *argf[] = {"./fileserv", NULL};

  if(auth_id == 0) execv(arga[0], arga);
  else
  {
    if((files_id =  fork()) == -1) //------ Fileserv de hijo
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
        //------ Lectura ------
        rw = recv_cmd(newfd,msg_buf);

        if(rw == -1)
        {
          perror("read");
          exit(EXIT_FAILURE);
        }
        //------ Fin Lectura ------

        if(!strcmp(msg_buf,"exit"))
        {
          close(newfd);
          buf.mtype = TO_AUTH;
          strcpy(buf.msg, "exit");

          msgsnd(msqid, &buf, sizeof(buf.msg), IPC_NOWAIT);

          buf.mtype = TO_FILE;
          msgsnd(msqid, &buf, sizeof(buf.msg), IPC_NOWAIT);

          if (msgctl(msqid, IPC_RMID, NULL) == -1) /* Destruye la cola */
          {
            perror("msgctl");
            exit(EXIT_FAILURE);
          }
          exit(EXIT_SUCCESS);
        }

        //------ Identificación y envío de mensaje ------
        if(strchr(msg_buf,',') != NULL)
        {
          buf.mtype = TO_AUTH;
          strcpy(buf.msg, msg_buf);

          msgsnd(msqid, &buf, sizeof(buf.msg), 0);

          msgrcv(msqid, &buf, sizeof(buf.msg), TO_PRIM, 0);
          send_client(rw, newfd, &buf);
        } else
        {
          int i = 0;
          char *comando[5], backup[STR_LEN];
          strcpy(backup, msg_buf);

          char *tmp = strtok(backup, " ");
          while(tmp != NULL)
          {
            comando[i] = tmp;
            i++;
            tmp = strtok(NULL," ");
          }

          switch (comando[0][0])
          {
            case USER:
              buf.mtype = TO_AUTH;
              if(!strcmp(comando[1],"ls"))
              {
                strcpy(buf.msg, comando[1]);
                msgsnd(msqid, &buf, sizeof(buf.msg), 0);

                int print = 0;
                msgrcv(msqid, &buf, sizeof(buf.msg), TO_PRIM, 0);
                sscanf(buf.msg, "%d", &print);
                print += 3;

                // sleep(2);
                // struct msqid_ds details;
                // msgctl(msqid, IPC_STAT, &details);
                // printf("Details: %ld\n", details.msg_qnum);
                // fflush(stdout);

                while(print != 0)
                {
                  msgrcv(msqid, &buf, sizeof(buf.msg), TO_PRIM, 0);
                  send_client(rw, newfd, &buf);
                  print--;
                  printf("%d\n", print);
                }

              }
              break;

            case FILE:

              break;
          }
        }
        //------ Fin de identificación y envío de mensaje ------


      }
    }
  }

  return EXIT_SUCCESS;
}

/**
 * @brief
 * Envía el mensaje recibido por los procesos hijos ('auth_service' o
 * 'files_service') al cliente.
 * @param rw    Resultado de la escritura en el socket.
 * @param newfd File descriptor perteneciente al socket.
 * @param buf   Mensaje a enviar.
 */
void send_client(ssize_t rw, int newfd, struct msg *buf)
{
  /**< Escritura */
  rw = send_cmd(newfd, buf->msg);

  if(rw == -1)
  {
    perror("write");
    exit(EXIT_FAILURE);
  }
  /**< Escritura */
}
