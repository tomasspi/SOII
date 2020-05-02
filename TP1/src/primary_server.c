/**
 * @file primary_server.c
 * @brief
 * Proceso con el rol de 'middleware'. Es el encargado de establecer las
 * conexiones y utiliza los procesos 'auth' y 'fileserv' para cumplir los
 * requerimientos del sistema.
 *
 * @author Tomás Santiago Piñero
 */

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "messages.h"
#include "sockets.h"
#include "utilities.h"

#define LS   "ls" /**< El comando 'ls'. */

/**
 * @brief
 * Enum con el destino del comando recibido.
 */
enum comando
{
  auth     = 1, /**< Destinado a 'auth_service'. */
  fileserv = 2  /**< Destinado a 'files_service'. */
};

struct msg buf; /**< Estrucutra de mensaje para la cola de mensajes. */
int msqid;      /**< ID de la cola de mensajes. */
int newfd;      /**< File descriptor de la conexión con el cliente. */
ssize_t rw = 0; /**< Resultado de lectura/escritura del socket. */

int  check_cmd(char buf[STR_LEN]);

void cmd_invalid(int newfd, char buf[STR_LEN]);

int  create_child(int child_id, int child_state, char *args[]);

int  create_queue();

void loop_listen(int sockfd, char msg_buf[STR_LEN], char *argv[]);

void parse_and_send_cmd(char msg_buf[STR_LEN]);

void print_ls(char cmd[1], char buff[STR_LEN], int newfd, int msqid,
              struct msg buf);

void send_client(int newfd, struct msg *buf);

void send_cmd_child(char *cmd[5], char msg_buf[STR_LEN]);

void send_message(long destino, struct msg buf, char msg_buf[STR_LEN]);

int  start_listening(int sockfd, char *argv[]);

/**
 * @brief
 * En caso de que ocurra algún error, se elimina la cola de mensajes.
 */
void delete_queue(void)
{
  if (msgctl(msqid, IPC_RMID, NULL) == -1) /* Destruye la cola */
    perror("msgctl");
}

int main(int argc, char *argv[])
{
  atexit(delete_queue);

  if(argc < 2)
    {
      fprintf(stderr, "Uso: %s Dirección IP\n", argv[0]);
      exit(EXIT_FAILURE);
    }

  int err = create_queue();
  check_error(err);

  int auth_id     = -1;
  int files_id    = -1;
  int auth_state  = -1;
  int files_state = -1;

  char *arga[] = {"./auth", NULL};
  char *argf[] = {"./fileserv", argv[1], NULL};

  err = create_child(auth_id, auth_state, arga);
  check_error(err);

  err = create_child(files_id, files_state, argf);
  check_error(err);

  int sockfd = -1;

  err = start_listening(sockfd, argv);
  check_error(err);

  char msg_buf[STR_LEN];

  loop_listen(sockfd, msg_buf, argv);

  return EXIT_SUCCESS;
}

/**
 * @brief
 * Loop en la que recibe los mensajes del cliente conectado.
 * @param sockfd  File descriptor de la conexión cliente-servidor.
 * @param msg_buf Buffer para almacenar los mensajes.
 * @param argv    Contiene la dirección IP del servidor.
 */
void loop_listen(int sockfd, char msg_buf[STR_LEN], char *argv[])
{
  int err;
  while(1)
    {
      rw = recv_cmd(newfd,msg_buf);
      check_error((int) rw);

      if(!strcmp(msg_buf,"exit"))
        {
          err = start_listening(sockfd, argv);
          check_error(err);

          rw = recv_cmd(newfd,msg_buf);
          check_error((int) rw);
        }

      if(is_login(msg_buf)) /* son credenciales? */
        {
          send_message(to_auth, buf, msg_buf);
          msgrcv(msqid, &buf, sizeof(buf.msg), to_prim, 0);
          send_client(newfd, &buf);
        }
      else /* es un comando */
        parse_and_send_cmd(msg_buf);
    }
}

/**
 * @brief
 * Parsea el mensaje enviado por el cliente y si es válido, lo envía a los
 * hijos.
 *
 * @param msg_buf Buffer que contiene el mensaje.
 */
void parse_and_send_cmd(char msg_buf[STR_LEN])
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

  if(comando[1] == NULL)
    cmd_invalid(newfd, msg_buf);

  send_cmd_child(comando, msg_buf);
}

/**
 * @brief
 * Envía al hijo correspondiente el comando.
 *
 * @param cmd     Comando a ejecutar.
 * @param msg_buf Buffer del mensaje enviado por el usuario.
 */
void send_cmd_child(char *cmd[5], char msg_buf[STR_LEN])
{
  switch (check_cmd(cmd[0]))
  {
    case auth:
            buf.mtype = to_auth;
            if(!strcmp(cmd[1],LS))
              print_ls(cmd[1], msg_buf, newfd, msqid, buf);

            else if(!strcmp(cmd[1],"passwd"))
              {
                if(!strcmp(cmd[2],""))
                  cmd_invalid(newfd, msg_buf);

                send_message(to_auth, buf, cmd[2]);
                msgrcv(msqid, &buf, sizeof(buf.msg), to_prim, 0);
                send_client(newfd, &buf);
              }
            else cmd_invalid(newfd, msg_buf);
            break;

    case fileserv:
            buf.mtype = to_file;
            if(!strcmp(cmd[1],LS))
              print_ls(cmd[1], msg_buf, newfd, msqid, buf);

            else if(!strcmp(cmd[1],"down"))
              {
                if(!strcmp(cmd[2],"") || !strcmp(cmd[3],""))
                  cmd_invalid(newfd, msg_buf);

                sprintf(buf.msg, "%s %s", cmd[2], cmd[3]);

                msgsnd(msqid, &buf, sizeof(buf.msg), 0);

                msgrcv(msqid, &buf, sizeof(buf.msg), to_prim, 0);
                send_client(newfd, &buf);
              }
            else cmd_invalid(newfd, msg_buf);
            break;

    default:
            cmd_invalid(newfd, msg_buf);
            break;
  }
}

/**
 * @brief
 * Determina si el comando enviado por el usuario es válido o no.
 *
 * @param  buf Comando recibido.
 * @return     Válido o inválido.
 */
int check_cmd(char buf[STR_LEN])
{
  if(!strcmp(buf,"user"))
    return auth;
  else if(!strcmp(buf,"file"))
    return fileserv;
  else
    return -1;
}

/**
 * @brief
 * Envía al proceso el comando recibido.
 *
 * @param destino Proceso hijo destino.
 * @param buf     Mensaje para la cola de mensajes.
 * @param msg_buf Buffer con el comando a ejecutar.
 */
void send_message(long destino, struct msg buf, char msg_buf[STR_LEN])
{
  buf.mtype = destino;
  strcpy(buf.msg, msg_buf);

  msgsnd(msqid, &buf, sizeof(buf.msg), 0);
}

/**
 * @brief
 * Función encargada de crear la cola de mensajes.
 * @return msqid ID de la cola de mensajes.
 */
int create_queue()
{
  key_t key;

  if ((key = ftok(QU_PATH, 5)) == -1)
    {
      perror("ftok");
      return -1;
    }

  msqid = msgget(key, 0666 | IPC_CREAT);

  if (msqid == -1)
    {
      perror("msgget");
      return -1;
    }

  return msqid;
}

/**
 * @brief
 * Función encargada de crear un proceso hijo.
 *
 * @param  child_id    ID del proceso hijo.
 * @param  child_state Estado del proceso hijo.
 * @param  args        Argumentos que recibe el proceso hijo.
 * @return             Fork exitoso (1) o fallido (-1)
 */
int create_child(int child_id, int child_state, char *args[])
{
  child_id = fork();

  if(child_id == -1)
    {
      perror("fork");
      return -1;
    }

  if(child_id == 0)
    execv(args[0], args);
  else
  {
    waitpid(child_id, &child_state, WNOHANG);
    return 1;
  }
  return 0;
}

/**
 * @brief
 * Función encargada de crear el socket y escuchar por posibles conexiones
 * al mismo.
 *
 * @param  sockfd File descriptor del socket.
 * @param  argv   Argumentos con la dirección IP.
 * @return        File descriptor cliente-servidor.
 */
int start_listening(int sockfd, char *argv[])
{
  sockfd = create_svsocket(argv[1], port_ps);

  struct sockaddr_in cl_addr;
  uint cl_len;
  char cl_ip[STR_LEN];

  printf("Esuchando en puerto %d...\n", port_ps);

  listen(sockfd, 1);
  cl_len = sizeof(cl_addr);

  newfd = accept(sockfd, (struct sockaddr *) &cl_addr, &cl_len);
  check_error(newfd);

  inet_ntop(AF_INET, &(cl_addr.sin_addr), cl_ip, INET_ADDRSTRLEN);
  printf("Conexión aceptada a %s\n", cl_ip);

  close(sockfd);
  return newfd;
}

/**
 * @brief
 * Envía el mensaje recibido por los procesos hijos ('auth_service' o
 * 'files_service') al cliente.
 * @param rw    Resultado de la escritura en el socket.
 * @param newfd File descriptor perteneciente al socket.
 * @param buf   Mensaje a enviar.
 */
void send_client(int newfd, struct msg *buf)
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
inline void cmd_invalid(int newfd, char buf[STR_LEN])
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
