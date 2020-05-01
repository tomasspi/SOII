#include <openssl/md5.h>

#include "utilities.h"
#include "messages.h"
/**
 * @brief
 * Función encargada de checkear el valor de operaciones que devuelvan -1 cuando
 * ocurre un error.
 *
 * @param err Número.
 */
inline void check_error(int err)
{
  if(err == -1)
    exit(EXIT_FAILURE);
}

/**
 * @brief
 * Función encargada de obtener la cola de mensajes.
 * @return msqid ID de la cola de mensajes.
 */
int get_queue()
{
  key_t key;

  if ((key = ftok(QU_PATH, 5)) == -1)
    {
      perror("ftok");
      return -1;
    }

  int msqid = msgget(key, 0666);

  if (msqid == -1)
    {
      perror("msgget");
      return -1;
    }

  return msqid;
}

/**
 * @brief
 * Función encargada de calcular el hash MD5 de un archivo. Si 'size' es 0,
 * entonces se leerá todo el archivo.
 * @param  path Path del USB.
 * @param  size Tamaño de cálculo.
 * @return      Hash MD5 del archivo.
 */
char *get_md5(char path[STR_LEN], size_t size)
{
  FILE *file = fopen(path, "rb");

  char buf[STR_LEN];
  size_t bytes, bytes_read;

  unsigned char c[MD5_DIGEST_LENGTH];
  MD5_CTX md_context;

  MD5_Init(&md_context);

  if(size == 0)
    {
      while((bytes = fread(buf, sizeof(char), sizeof(buf), file)) != 0)
        MD5_Update(&md_context, buf, bytes);
    }
  else
    {
      bytes = 0;
      while(bytes < size)
        {
          bytes_read = fread(buf, sizeof(char), sizeof(buf), file);
          MD5_Update(&md_context, buf, bytes_read);
          bytes += bytes_read;
        }
    }

  MD5_Final(c, &md_context);

  char *md5 = malloc(MD5_DIGEST_LENGTH * 2 + 1);

  for(int32_t i = 0; i < MD5_DIGEST_LENGTH; i++)
    sprintf(&md5[i*2], "%02x", (uint) c[i]);

  return md5;
}

/**
 * @brief
 * Indica si el mensaje enviado son credenciales de login o un comando.
 * @param  msg Mensaje recibido.
 * @return     Si son credenciales o no.
 */
bool is_login(char msg[MAX])
{
  if(strstr(msg,",") != NULL)
    return true;
  else return false;
}
