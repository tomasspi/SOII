/**
 * @file files_service.c
 * @brief
 * Proceso encargado de proveer las funcionalidades relacionadas a las
 * imágenes disponibles para decargar y la transferencia de la imagen
 * seleccionada.
 *
 * @author Tomás Santiago Piñero
 */

#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>

#include "messages.h"
#include "sockets.h"
#include "utilities.h"

#define IMGS_PATH "../imgs"

void print_images(int msqid, struct msg buf);

void send_image(char img[STR_LEN], long *size, int newfd);

void send_img_data(char img[STR_LEN], long *size, struct msg buf, int msqid);

int start_listening(int sockfd, char *ip);

int main(int argc, char *argv[])
{
  if(argc < 2)
    {
      fprintf(stderr, "Uso: %s Dirección IP\n", argv[1]);
      exit(EXIT_FAILURE);
    }

  printf("Files service started.\n");

  struct msg buf;
  int err;

  err = get_queue();
  check_error(err);
  int msqid = err;

  while (1)
  {
    msgrcv(msqid, &buf, sizeof(buf.msg), to_file, 0);

    if(!strcmp(buf.msg,"ls"))
      {
        buf.mtype = to_prim;
        print_images(msqid, buf);
      }
    else
      {
        char img[STR_LEN];
        long size;
        send_img_data(img, &size, buf, msqid);

        int fifd = -1;
        err = start_listening(fifd, argv[1]);
        check_error(err);
        fifd = err;

        send_image(img, &size, fifd);
      }
  }

  return EXIT_SUCCESS;
}

/**
 * @brief
 * Función encargada de crear el socket y conectarse al cliente.
 *
 * @param  sockfd File descriptor del socket.
 * @param  ip     Dirección IP del servidor.
 * @return        File descriptor cliente-servidor.
 */
int start_listening(int sockfd, char *ip)
{
  sockfd = create_svsocket(ip, port_fi);

  struct sockaddr_in cl_addr;
  uint cl_len;
  char cl_ip[STR_LEN];

  printf("Esuchando en puerto %d...\n", port_fi);

  listen(sockfd, 1);
  cl_len = sizeof(cl_addr);

  int newfd;
  newfd = accept(sockfd, (struct sockaddr *) &cl_addr, &cl_len);
  check_error(newfd);

  inet_ntop(AF_INET, &(cl_addr.sin_addr), cl_ip, INET_ADDRSTRLEN);
  printf("Conexión aceptada a %s\n", cl_ip);

  close(sockfd);
  return newfd;
}

/**
 * @brief
 * Función encargada de realizar el envío de la imagen al cliente.
 *
 * @param img   Imagen pedida por el cliente.
 * @param size  Tamaño de la imagen seleccionada.
 * @param newfd File descriptor de la conexión.
 */
void send_image(char img[STR_LEN], long *size, int newfd)
{
  int32_t imgfd;

  imgfd = open(img, O_RDONLY);
  check_error(imgfd);

  printf("%s\n", "  FS: Sending file...");

  size_t to_send = (size_t) size;
  ssize_t sent;
  off_t offset = 0;

  while(((sent = sendfile(newfd, imgfd, &offset, to_send)) > 0)
        && (to_send > 0))
  {
    to_send -= (size_t) sent;
  }

  printf("  FS: %lu %s\n", *size, "sent.");

  close(imgfd);
}

/**
 * @brief
 * Envía al cliente los datos de la imagen seleccionada.
 *
 * @param img   Imagen seleccionada
 * @param size  Tamaño de la imagen.
 * @param buf   Mensaje para 'primary_server'.
 * @param msqid ID de la cola de mensajes.
 */
void send_img_data(char img[STR_LEN], long *size, struct msg buf, int msqid)
{
  buf.mtype = to_prim;

  char *tok = strtok(buf.msg, " ");

  strcpy(img, IMGS_PATH); /* Nombre de la imagen */
  strcat(img, "/");
  strcat(img, tok);

  tok = strtok(NULL, " "); /* Dispositivo a escribir */
  char usb[STR_LEN];
  strcpy(usb,tok);

  FILE *imgn;
  imgn = fopen(img, "r");

  if(imgn == NULL)
    perror("file");

  fseek(imgn, 0, SEEK_END); /* Calcula el tamaño de la imagen */
  *size = ftell(imgn);
  fclose(imgn);

  char size_s[STR_LEN] = "";
  sprintf(size_s, "%ld", *size);

  char *md5s = get_md5(img, 0);

  sprintf(buf.msg, "Download %s %s %s", size_s, usb, md5s);

  msgsnd(msqid, &buf, sizeof(buf.msg), 0);
}

/**
 * @brief
 * Función encargada de imprimir las imágenes disponibles para descargar.
 * @param msqid ID de la cola de mensajes.
 * @param buf   Estructura para enviar los mensajes.
 */
void print_images(int msqid, struct msg buf)
{
  DIR *dir;
  struct dirent *direct;
  char msg[STR_LEN] = "";

  dir = opendir(IMGS_PATH);

  if(dir == NULL)
    perror("print imgs directory");

  strcat(msg, "=====================================================================================\n");

  sprintf(buf.msg, "%-35s %-15s %-15s\n", "Imagen", "Tamaño [MB]", "MD5");
  strcat(msg, buf.msg);

  strcat(msg, "=====================================================================================\n");

  while ((direct = readdir(dir)) != NULL)
  {
    if (direct->d_type == DT_REG)
    {
      long size = 0;

      FILE *file;
      char path[100];
      strcpy(path, IMGS_PATH);
      strcat(path,"/");
      strcat(path,direct->d_name);

      file = fopen(path, "r");
      if(file == NULL)
        perror("file");

      //------ Tamaño del archivo ------
      fseek(file, 0, SEEK_END);
      size = ftell(file);
      size /= 1000000;
      fclose(file);

      char *md5 = get_md5(path, 0);

      sprintf(buf.msg, "%-35s %-14ld %-15s\n", direct->d_name, size, md5);
      strcat(msg, buf.msg);
    }
  }
  closedir(dir);
  strcpy(buf.msg, msg);
  msgsnd(msqid, &buf, sizeof(buf.msg), 0);

  memset(msg, '\0', STR_LEN-1);
  memset(buf.msg, '\0', sizeof(buf.msg));
}
