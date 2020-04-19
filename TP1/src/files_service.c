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
#include <netinet/in.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <fcntl.h>

#include "messages.h"
#include "sockets.h"

#define IMGS_PATH "../imgs"

void print_images(int msqid, struct msg buf);
int get_images();

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

        strcpy(img, IMGS_PATH);
        strcat(img, "/");
        strcat(img, buf.msg);
  //------ Crea el nuevo socket ------
        int fifd;

create:
        fifd = create_svsocket(port_fi);
        int newfd;

        if(fifd == -1)
          {
            perror("Files socket");
            goto create;
          }

        struct sockaddr_in cl_addr;
        uint cl_len;

        printf("FS: Esuchando en puerto %d...\n", port_fi);

        listen(fifd, 1);
        cl_len = sizeof(cl_addr);

  accept:
        newfd = accept(fifd, (struct sockaddr *) &cl_addr, &cl_len);

        if(newfd == -1)
          {
            perror("accept");
            goto accept;
          }

        printf("FS: Conexión aceptada.\n");

        close(fifd);
  //------ Fin creación socket y conexión ------
        FILE *imgn;
        long size;
        off_t offset = 0;

        imgn = fopen(img, "r");

        if(imgn == NULL)
          perror("file");

        //------ Tamaño del archivo ------
        fseek(imgn, 0, SEEK_END);
        size = ftell(imgn);
        fclose(imgn);

        char size_s[STR_LEN] = "";
        sprintf(size_s, "%ld",size);

        strcpy(buf.msg, "Download ");
        strcat(buf.msg, size_s);
        msgsnd(msqid, &buf, sizeof(buf.msg), 0);

        int32_t img_to_send;

retry:
        img_to_send = open(img, O_RDONLY);

        if(img_to_send == -1)
          {
            perror("open2send");
            goto retry;
          }

        if(sendfile(fifd, img_to_send, &offset,(size_t) size) == -1)
          {
            perror("sendfile");
            goto retry;
          }
        else
          {
            close(img_to_send);
            printf("%s\n", "Envío completado.");
          }
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
      if(file == NULL) perror("file");

      //------ Tamaño del archivo ------
      fseek(file, 0, SEEK_END);
      size = ftell(file);
      size /= 1000000;

      //------ MD5 del archivo ------
      unsigned char c[MD5_DIGEST_LENGTH];
      MD5_CTX mdContext;
      size_t bytes;
      unsigned char data[1024];
      MD5_Init(&mdContext);

      while ((bytes = fread(data, 1, 1024, file)) != 0)
          MD5_Update(&mdContext, data, bytes);

      MD5_Final(c,&mdContext);

      //------ MD% to string ------
      char md5[33];
      for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(&md5[i*2], "%02x",(unsigned int) c[i]);

      fclose(file);

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
