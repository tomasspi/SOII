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
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <fcntl.h>

#include "messages.h"
#include "sockets.h"

#define IMGS_PATH "../imgs"

void print_images(int msqid, struct msg buf);
int get_images();
char *get_md5(char *path);

int main(int argc, char *argv[])
{
  if(argc < 2)
    {
      fprintf(stderr, "Uso: %s Dirección IP\n", argv[1]);
      exit(EXIT_FAILURE);
    }

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

  buf.mtype = to_prim;

  while (1)
  {
    msgrcv(msqid, &buf, sizeof(buf.msg), to_file, 0);

    if(!strcmp(buf.msg, "exit"))
      exit(EXIT_SUCCESS);

    if(!strcmp(buf.msg,"ls"))
      {
        buf.mtype = to_prim;
        print_images(msqid, buf);
      }
    else
      {
        buf.mtype = to_prim;
        char img[STR_LEN];

        char *tok = strtok(buf.msg, " ");

        strcpy(img, IMGS_PATH);
        strcat(img, "/");
        strcat(img, tok);

        tok = strtok(NULL, " ");
        char usb[STR_LEN];
        strcpy(usb,tok);

  //------ Crea el nuevo socket ------
        int fifd  = create_svsocket(argv[1], port_fi);
        int newfd;

        if(fifd == -1)
          {
            perror("Files socket");
            exit(EXIT_FAILURE);
          }
  //------ Fin creación socket ------
        FILE *imgn;
        long size;

        imgn = fopen(img, "r");

        if(imgn == NULL)
          perror("file");

        //------ Tamaño del archivo ------
        fseek(imgn, 0, SEEK_END);
        size = ftell(imgn);
        fclose(imgn);

        char size_s[STR_LEN] = "";
        sprintf(size_s, "%ld", size);

        char *md5s = get_md5(img);

        sprintf(buf.msg, "Download %s %s %s", size_s, usb, md5s);

        msgsnd(msqid, &buf, sizeof(buf.msg), 0);

  //------ Espera conexión de cliente ------
        struct sockaddr_in cl_addr;
        uint cl_len;

        printf("  FS: Esuchando en puerto %d...\n", port_fi);

        listen(fifd, 1);
        cl_len = sizeof(cl_addr);

        newfd = accept(fifd, (struct sockaddr *) &cl_addr, &cl_len);

        if(newfd == -1)
          {
            perror("accept");
            exit(EXIT_FAILURE);
          }

        printf("  FS: Conexión aceptada.\n");
        close(fifd);
  //------ Cliente conectado ------
        int32_t imgfd;

        imgfd = open(img, O_RDONLY);

        if(imgfd == -1)
          {
            perror("open image");
            exit(EXIT_FAILURE);
          }

        printf("%s\n", "  FS: Sending file...");

        size_t to_send = (size_t) size;
        ssize_t sent;
        off_t offset = 0;

        while(((sent = sendfile(newfd, imgfd, &offset, to_send)) > 0)
              && (to_send > 0))
        {
          to_send -= (size_t) sent;
        }

        printf("  FS: %lu %s\n", size, "sent.");

        close(imgfd);
      }
  }

  return EXIT_SUCCESS;
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

      char *md5 = get_md5(path);

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

/**
 * @brief
 * Función encargada de calcular el hash MD5 del archivo.
 * @param file     Archivo al cual se desea calcular el hash MD5.
 * @param md5[33]  String donde se guarda el hash MD5.
 */
char *get_md5(char *path)
{
  FILE *file = fopen(path, "rb");
  char buf[STR_LEN];
  size_t bytes;

  //------ MD5 del archivo ------
  unsigned char c[MD5_DIGEST_LENGTH];
  MD5_CTX mdContext;

  MD5_Init(&mdContext);

  while ((bytes = fread(buf, sizeof(char), sizeof(buf), file)) != 0)
    MD5_Update(&mdContext, buf, bytes);

  MD5_Final(c,&mdContext);

  char *md5= malloc(MD5_DIGEST_LENGTH * 2 + 1);

  //------ MD5 to string ------
  for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
    sprintf(&md5[i*2], "%02x",(unsigned int) c[i]);

  return md5;
}
