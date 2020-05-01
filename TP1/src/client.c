/**
 * @file client.c
 * @brief
 * Se conecta al servidor para descargar una imagen de SO y escribirla en un
 * USB.
 *
 * @author Tomás Santiago Piñero
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>

#include "sockets.h"
#include "utilities.h"

void ask_login(char buf[STR_LEN]);
void little_to_big(char little[4], char big[8]);
void show_mbr(char path[STR_LEN]);
void show_prompt(int32_t sockfd, ssize_t rw, char buffer[STR_LEN], char ip[STR_LEN]);

int32_t sockfd;

struct mbr            /** Estructura para leer la tabla MBR. */
{
  char boot[1];       /** Indica si es 'booteable'. */
  char start_chs[3];  /** Comienzo de CHS. */
  char type[1];       /** Tipo de partición. */
  char end_chs[3];    /** Final de CHS. */
  char start[4];      /** Sector de arranque de la partición. */
  char size[4];       /** Tamaño de la partición (en sectores). */
} __attribute__((__packed__));

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

int main(int argc, char *argv[])
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

  if(argc < 2)
    {
      fprintf(stderr, "Uso: %s Dirección IP del servidor\n", argv[0]);
      exit(EXIT_FAILURE);
    }

  int32_t tries = 3;

  char buffer[STR_LEN];

	sockfd = create_clsocket(argv[1], port_ps);
  ssize_t rw;

  do
    {
      ask_login(buffer);

send: rw = send_cmd(sockfd,buffer);

      if(rw == -1)
        {
          perror("write");
          goto send;
        }

      rw = recv_cmd(sockfd, buffer);

      if(strchr(buffer,'/') != NULL)
        {
          printf("\nLast login: %s\n", buffer);
          show_prompt(sockfd, rw, buffer, argv[1]);
        }
      else
        {
          printf("%s\n", buffer);
          tries--;
        }
    }
  while(tries > 0);

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
get_user:
  printf("Ingrese usuario: ");

  if(fgets(buf, STR_LEN, stdin) == NULL)
    {
      perror("fgets login");
      goto get_user;
    }

  if(buf[0] == '\n')
    goto get_user;
  else buf[strlen(buf)-1] = ',';

  size_t coma = strlen(buf)-1;

get_pass:
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

  if((strlen(buf)-1) == coma)
    goto get_pass;
}

/**
 * @brief
 * Función encargada de mostrar el 'prompt' al usuario en caso de ser validadas
 * las credenciales enviadas.
 * @param sockfd File descriptor del socket al servidor.
 * @param rw     Resultado de la operación de Lectura/escritura.
 * @param buffer Mensaje enviado/recibido.
 */
void show_prompt(int32_t sockfd, ssize_t rw, char buffer[STR_LEN], char ip[STR_LEN])
{
  prompt:

  while(1)
    {
  		printf("#> ");
  		memset(buffer, '\0', STR_LEN);
  		if(fgets(buffer, STR_LEN-1, stdin) == NULL) perror("fgets prompt");
      buffer[strlen(buffer)-1] = '\0';

      if(buffer[0] == '\0')
        goto prompt;

  		rw = send_cmd(sockfd, buffer);

      if(rw == -1)
        {
          perror("write");
          goto prompt;
        }

      if(!strcmp(buffer, "exit"))
        exit(EXIT_SUCCESS);

  read:
      rw = recv_cmd(sockfd, buffer);

      if(rw == -1)
        {
          perror("read");
          goto read;
        }

      if(strstr(buffer,"Download") != NULL)
        {
          char *tok = strtok(buffer, " ");
          tok = strtok(NULL, " ");

          size_t size = 0;
          size_t size_for_md5;
          sscanf(tok, "%lud", &size);
          size_for_md5 = size;

          int32_t fifd = create_clsocket(ip, port_fi);

          printf("Writing...\n");

          FILE *usb;
          char path_usb[STR_LEN];
          tok = strtok(NULL, " ");
          strcpy(path_usb,tok);

          char md5_recv[33];
          tok = strtok(NULL, " ");
          sprintf(md5_recv, "%s", tok);

          usb = fopen(path_usb, "wb");

          if(usb == NULL)
            {
              perror("open usb");
              exit(EXIT_FAILURE);
            }

          do
            {
              rw = recv(fifd, buffer, STR_LEN, 0);
              if(rw == -1)
                perror("recv file");

              fwrite(buffer, sizeof(char), (size_t) rw, usb);

              size -= (size_t) rw;
            }
          while(size != 0);

          sync();
          fclose(usb);

          printf("Done writing.\n");

          char *md5 = get_md5(path_usb, size_for_md5);

          //------ Termina de escribir y realiza la verificación del hash ------
          if(!strcmp(md5_recv, md5))
            {
              printf("Escritura exitosa.\n");
              show_mbr(path_usb);
            }
          else
            printf("Escritura fallida.\n");

          close(fifd);
        }
      else printf("%s\n", buffer);
  	}
}

/**
 * @brief
 * Función encargada de leer la tabla MBR y mostrar sus contenidos.
 * @param path_usb[STR_LEN] Path al USB.
 */
void show_mbr(char path_usb[STR_LEN])
{
  FILE *usb = fopen(path_usb, "rb");

  struct mbr table;

  if(usb != NULL)
    {
      fseek(usb, 0L, SEEK_SET);
      fseek(usb, 446, SEEK_CUR);

      if(fread(&table, sizeof(table), 1, usb) > 0)
        fclose(usb);
    }
  else
    {
      perror("read usb");
      exit(EXIT_FAILURE);
    }

  printf("\n================================\n");
  char boot[3];
  sprintf(boot, "%02x", table.boot[0] & 0xff);

  if(!strcmp(boot,"80"))
    printf(" - Booteable: Si.\n");
  else
    printf(" - Booteable: No.\n");

  char type[3];
  sprintf(type, "%02x", table.type[0] & 0xff);

  printf(" - Tipo de partición: %s\n", type);

  char start[8] = "\0";
  char size[8]  = "\0";

  little_to_big(start, table.start);

  long inicio = strtol(start, NULL, 16);
  if (errno == ERANGE)
    printf("Over/underflow %ld\n", inicio);

  printf(" - Sector de inicio: %ld \n", inicio);

  little_to_big(size, table.size);

  long tamanio = strtol(size, NULL, 16);
  if (errno == ERANGE)
    printf("Over/underflow %ld\n", tamanio);

  tamanio *= 512;
  tamanio /= 1000000;

  printf(" - Tamaño de la partición: %ld MB\n\n", tamanio);
}

/**
 * @brief
 * Función encargada de convertir valores de 4 bytes guardados en little endian
 * a big endian.
 * @param big    Valor en big endian.
 * @param little Valor en little endian.
 */
void little_to_big(char big[8], char little[4])
{
  char byte[3];
  for(int i = 3; i >= 0; i--)
  {
    sprintf(byte, "%02x", little[i] & 0xff);
    strcat(big, byte);
  }
}
