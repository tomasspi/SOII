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

#define LS   "ls" /**< El comando 'ls'. */

int newfd;
struct msg buf;
int msqid;

void cmd_invalid(int newfd, char buf[STR_LEN]);
void print_ls(char cmd[1], char buff[STR_LEN], int newfd, int msqid,
              struct msg buf);
void send_client(ssize_t rw, int newfd, struct msg *buf);

/**
 * @brief
 * En caso de que ocurra algún error, se envía a los procesos hijos un 'exit'
 * para que finalicen su ejecución.
 */
void kill_childs(void)
{
  close(newfd);
  buf.mtype = to_auth;
  strcpy(buf.msg, "exit");

  msgsnd(msqid, &buf, sizeof(buf.msg), IPC_NOWAIT);

  buf.mtype = to_file;
  msgsnd(msqid, &buf, sizeof(buf.msg), IPC_NOWAIT);

  if (msgctl(msqid, IPC_RMID, NULL) == -1) /* Destruye la cola */
    perror("msgctl");
}

int main(int argc, char *argv[])
{
  atexit(kill_childs);

  if(argc < 2)
    {
      fprintf(stderr, "Uso: %s Dirección IP\n", argv[0]);
      exit(EXIT_FAILURE);
    }

  int sockfd = create_svsocket(argv[1], port_ps);

  ssize_t rw;
  char msg_buf[STR_LEN];

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

  printf("Esuchando en puerto %d...\n", port_ps);

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

  int auth_id, files_id, auth_state, files_state;

  if((auth_id = fork()) == -1) //------ Auth de hijo ------
    {
      perror("fork auth");
      exit(EXIT_FAILURE);
    }

  char *arga[] = {"./auth", NULL};
  char *argf[] = {"./fileserv", argv[1], NULL};

  if(auth_id == 0)
    execv(arga[0], arga);
  else
    {
      if((files_id =  fork()) == -1) //------ Fileserv de hijo ------
        {
          perror("fork files");
          exit(EXIT_FAILURE);
        }

      if(files_id == 0)
        execv(argf[0], argf);
      else
        {
          waitpid(auth_id, &auth_state, WNOHANG);
          waitpid(files_id, &files_state, WNOHANG);

    listen:
          while(1)
            {
              //------ Lectura ------
              rw = recv_cmd(newfd,msg_buf);

              if(rw == -1)
                {
                  perror("read");
                  goto listen;
                }
              //------ Fin Lectura ------

              if(!strcmp(msg_buf,"exit"))
                exit(EXIT_SUCCESS);

              //------ Identificación y envío de mensaje ------
              if(strchr(msg_buf,',') != NULL) //------ Inicio de sesión ------
                {
                  buf.mtype = to_auth;
                  strcpy(buf.msg, msg_buf);

                  msgsnd(msqid, &buf, sizeof(buf.msg), 0);

                  msgrcv(msqid, &buf, sizeof(buf.msg), to_prim, 0);
                  send_client(rw, newfd, &buf);
                }
              else //------ Es un comando ------
                {
                  int i = 0;
                  char *comando[5], backup[STR_LEN];
                  strcpy(backup, msg_buf);

                  for(int i = 0; i < 5; i++)
                    comando[i] = "";

                  char *tmp = strtok(backup, " ");
                  while(tmp != NULL)
                    {
                      comando[i] = tmp;
                      i++;
                      tmp = strtok(NULL," ");
                    }

                  if(comando[1] == NULL)
                    {
                      cmd_invalid(newfd, msg_buf);
                      goto listen;
                    }

                  if(!strcmp(comando[0], "user"))
                    {
                      buf.mtype = to_auth;
                      if(!strcmp(comando[1],LS))
                        print_ls(comando[1], msg_buf, newfd, msqid, buf);

                      else if(!strcmp(comando[1],"passwd"))
                        {
                          if(!strcmp(comando[2],""))
                            goto invalid;

                          strcpy(buf.msg, comando[2]);

                          msgsnd(msqid, &buf, sizeof(buf.msg), 0);

                          msgrcv(msqid, &buf, sizeof(buf.msg), to_prim, 0);
                          send_client(rw, newfd, &buf);
                        }
                      else goto invalid;
                    }
                  else if(!strcmp(comando[0], "file"))
                    {
                      buf.mtype = to_file;
                      if(!strcmp(comando[1],LS))
                        print_ls(comando[1], msg_buf, newfd, msqid, buf);

                      else if(!strcmp(comando[1],"down"))
                        {
                          if(!strcmp(comando[2],"") || !strcmp(comando[3],""))
                            goto invalid;

                          sprintf(buf.msg, "%s %s", comando[2], comando[3]);

                          msgsnd(msqid, &buf, sizeof(buf.msg), 0);

                          msgrcv(msqid, &buf, sizeof(buf.msg), to_prim, 0);
                          send_client(rw, newfd, &buf);
                        }
                      else goto invalid;
                    }
                  else
invalid:           cmd_invalid(newfd, msg_buf);
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
  rw = send_cmd(newfd, buf->msg);

  if(rw == -1)
  {
    perror("write");
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief
 * Función encargada de decir al cliente que el comando ingresado no es válido.
 * @param newfd File descriptor perteneciente al socket.
 * @param buf   Mensaje a enviar.
 */
void cmd_invalid(int newfd, char buf[STR_LEN])
{
  sprintf(buf,"Comando inválido. "
          "Los comandos soportados son:\n - %s\n - %s\n - %s\n - %s\n",
          "user ls", "user passwd 'nueva_password'",
          "file ls", "file down 'nombre_imagen' 'directorio_usb'");

  send_cmd(newfd, buf);
}

/**
 * @brief
 * Función encargada de enviar el comando 'ls' a cualquiera de los dos servicios
 * y esperar la respuesta del mismo.
 * @param cmd    Comando a enviar (ls).
 * @param buff   Mensaje para enviar al cliente.
 * @param newfd  File descriptor perteneciente al socket client-server.
 * @param msqid  ID de la cola de mensajes.
 * @param buf    Mensajes de la cola.
 */
void print_ls(char cmd[1], char buff[STR_LEN], int newfd, int msqid, struct msg buf)
{
  memset(buff, '\0', STR_LEN-1);
  strcpy(buf.msg, cmd);
  msgsnd(msqid, &buf, sizeof(buf.msg), 0);

  msgrcv(msqid, &buf, sizeof(buf.msg), to_prim, 0);

  if(send_cmd(newfd, buf.msg) == -1)
    perror("write ls");
}
