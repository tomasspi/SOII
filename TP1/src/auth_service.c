/**
 * @file auth_service.c
 * @brief
 * Proceso encargado de proveer las funcionalidades asociadas al manejo de la
 * base de datos de usuarios.
 *
 * @author Tomás Santiago Piñero
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <sys/msg.h>

#include "messages.h"

#define COLUMNAS  5            /**< Cantidad de columnas de la BD. */
#define BLOCKED_Q "bloqueado"  /**< ¿El usuario está bloqueado? */
#define DB_PATH   "../res/usuarios_database.txt" /**< Path a la BD.*/
#define TMP_PATH  "../res/DB.tmp" /**< Path a la BD temporal. */
#define FECHA     'F'          /**< Utilizada para cambiar fecha en BD. */
#define PASSWD    'P'          /**< Utilizado para cambiar password en BD. */
#define STATUS    'S'          /**< Utilizado para cambiar el estado en BD. */
#define LINE_LEN  80           /**< Largo de la linea a leer. */

void login(char buf[STR_LEN], char credentials[2][STR_LEN]);
void get_date(char date[20]);
void print_database(int msqid, struct msg buf);
int  write_database(char data[COLUMNAS][STR_LEN], const char t, const char *new);
void replace(char *str, const char *old, const char *new);
bool check_database(char input[2][STR_LEN], char database[COLUMNAS][STR_LEN]);
int  check_credentials(char credentials[2][STR_LEN], char data[COLUMNAS][STR_LEN]);

char date[20];  /**< Fecha actual. */

int main(void)
{
  printf("Authentication service started.\n");

  char input[2][STR_LEN];           /**< Almacena las credenciales recibidas. */
  char database[COLUMNAS][STR_LEN]; /**< Almacena los datos obtenidos de la BD. */

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

  int tries = 3;
  int auth = 0;

  while(1)
    {
      memset(buf.msg, '\0', sizeof(buf.msg));
      msgrcv(msqid, &buf, sizeof(buf.msg), to_auth, 0);

      if(strstr(buf.msg,",") != NULL)
      {
        do
          {
            login(buf.msg, input);

            if(check_database(input, database))
              {
                switch(check_credentials(input, database))
                  {
                    case blocked:
                      strcpy(buf.msg, "El usuario está bloqueado.\n");
                      break;

                    case active:
                      strcpy(buf.msg, database[COLUMNAS-1]);
                      get_date(date);

                      if(write_database(database, FECHA, date) != 1)
                        perror("write_database");

                      auth = 1;
                      break;

                    case invalid:
                      strcpy(buf.msg, "Credenciales inválidas.\n");
                      tries--;

                      if(tries == 0)
                        {
                          strcpy(buf.msg, "El usuario ha sido bloqueado.\n");
                          if(write_database(database, STATUS, BLOCKED_Q) != 1)
                            perror("write_database");
                        }
                      break;
                  }
              } else strcpy(buf.msg,"El usuario no existe.\n");

            buf.mtype = to_prim;
            msgsnd(msqid, &buf, sizeof(buf.msg), 0);
          }
        while(tries && !auth);
      }
      else
       {
        if(!strcmp(buf.msg, "exit"))
          {
            auth = 0;
            tries = 3;
          }

        if(!strcmp(buf.msg,"ls"))
          {
            buf.mtype = to_prim;
            print_database(msqid, buf);
          }
        else
          {
            buf.mtype = to_prim;

            if(!strcmp(buf.msg, database[1]))
              {
                strcpy(buf.msg, "User and password can't be the same.\n");
                msgsnd(msqid, &buf, sizeof(buf.msg), 0);
              }
            else
              {
                if(write_database(database, PASSWD, buf.msg) != 1)
                  perror("write_database");

                else
                  {
                    strcpy(database[COLUMNAS-3],buf.msg);

                    strcpy(buf.msg, "Password changed successfully.\n");
                    msgsnd(msqid, &buf, sizeof(buf.msg), 0);
                  }
              }
          }
        }
    }

  return EXIT_SUCCESS;
}

/**
 * @brief
 * Función encargada de tomar las credenciales (usuario y contraseña) para el
 * login. Estas credenciales se guardan en la estructura 'log_msg' con el
 * fin de enviar las credenciales a 'auth_service'.
 * @param buf[STR_LEN]            Mensaje con las credenciales del usuario.
 * @param credentials[2][STR_LEN] Credenciales separadas.
 */
void login(char buf[STR_LEN], char credentials[2][STR_LEN])
{
  char *tok;
  tok = strtok(buf,",");

  strcpy(credentials[0],tok);

  tok = strtok(NULL,"");

  strcpy(credentials[1],tok);
}

/**
 * @brief
 * Función encargada de verificar si el usuario ingresado se encuentra
 * almacenado en la base de datos (usuarios_database.txt). En caso de que el
 * usuario exista, se guardan los datos obtenidos en el arreglo 2D 'database'.
 * @param  input[2][STR_LEN]           Credenciales ingresadas por el usuario.
 * @param  database[COLUMNAS][STR_LEN] Datos obtenidos de la BD.
 * @return                             Si el usuario ingresado existe en la BD.
 */
bool check_database(char input[2][STR_LEN], char database[COLUMNAS][STR_LEN])
{
  char string[LINE_LEN];
  char *tmp, *contenido[COLUMNAS];
  int i;

  FILE *archivo = fopen(DB_PATH, "r");

  if(archivo == NULL)
    perror("Archivo DB");


  while(fgets(string, LINE_LEN, archivo) != NULL)
    {
      //reemplazo el salto de lina con fin de linea
      if(strlen(string) > 0 && string[strlen(string)-1] == '\n')
        string[strlen(string)-1] = '\0';

      i = 0;
      tmp = strtok(string,",");

      //Leo archivo
      while(tmp != NULL)
        {
          contenido[i] = tmp;
          i++;
          tmp = strtok(NULL,",");
        }

      //encontre usuario?
      if(!strcmp(input[0],contenido[1]))
        {
          fclose(archivo);
          //Copio a database
          for(int j = 0; j < COLUMNAS; j++)
            strcpy(database[j],contenido[j]);

          return true;
        }
    }
  fclose(archivo);
  return false;
}

/**
 * @brief
 * Función encargada de verificar las credenciales ingresadas con las que
 * fueron obtenidas de la base de datos. En caso de que las credenciales sean
 * válidas, modifica la última fecha de acceso por medio de la función
 * 'print_database'.
 * @param  credentials[2][STR_LEN] Credenciales ingresadas por el usuario.
 * @param  data[COLUMNAS][STR_LEN] Datos obtenidos de la BD.
 * @return                         Si el usuario está bloqueado o si la constraseña
 *                                 es correcta o no.
 */
int check_credentials(char credentials[2][STR_LEN], char data[COLUMNAS][STR_LEN])
{
  if(!strcmp(credentials[1],data[2]))
    {
      if(!strcmp(data[COLUMNAS-2],BLOCKED_Q))
        return blocked;
      else return active;
    }
  else return invalid;
}

/**
 * @brief
 * Función encargada de imprimir los datos que se encuentran registrados en la
 * base de datos (usuarios_database.txt).
 * @param msqid ID de la cola de mensajes.
 * @param buf   Estructura para enviar los mensajes.
 */
void print_database(int msqid, struct msg buf)
{
  char string[LINE_LEN];
  char *tmp, *contenido[COLUMNAS], msg[STR_LEN] = "";
  int i;

  FILE *archivo = fopen(DB_PATH, "r");

  if(archivo == NULL)
    perror("Archivo DB");

  strcat(msg, "=====================================================\n");

  sprintf(buf.msg, "%-15s %-15s %-15s\n", "Usuario", "Estado", "Último login");
  strcat(msg, buf.msg);

  strcat(msg, "=====================================================\n");

  while(fgets(string, LINE_LEN, archivo) != NULL)
    {
      //reemplazo el salto de lina con fin de linea
      if(strlen(string) > 0 && string[strlen(string)-1] == '\n')
        string[strlen(string)-1] = '\0';

      i = 0;
      tmp = strtok(string,",");

      //Leo archivo
      while(tmp != NULL)
      {
        contenido[i] = tmp;
        i++;
        tmp = strtok(NULL,",");
      }

      sprintf(buf.msg, "%-15s %-15s %-15s\n",
              contenido[1], contenido[3], contenido[4]);

      strcat(msg, buf.msg);
    }

  fclose(archivo);
  strcpy(buf.msg, msg);
  msgsnd(msqid, &buf, sizeof(buf.msg), 0);

  memset(msg, '\0', STR_LEN-1);
  memset(buf.msg, '\0', sizeof(buf.msg));
}

/**
 * @brief
 * Función encargada de modificar los datos que se encuentran registrados en la
 * base de datos (usuarios_database.txt).
 * @param data[COLUMNAS][STR_LEN]  Datos del usuario a modificar.
 * @param t                        Valor a modificar (FECHA, STATUS o PASSWD).
 * @param new                      Nuevo valor.
 */
int write_database(char data[COLUMNAS][STR_LEN], const char t, const char *new)
{
  char string[LINE_LEN];
  FILE *archivo, *reemplazo;

  archivo = fopen(DB_PATH, "r");
  reemplazo = fopen(TMP_PATH, "w");

  if(archivo == NULL || reemplazo == NULL)
    perror("write_database");

  while(fgets(string, LINE_LEN, archivo) != NULL)
    {
      //Busco usuario por ID.
      if(!strncmp(string,data[0],1))
        {
          switch (t)
            {
              case FECHA:
                replace(string, data[COLUMNAS-1], new);
                fputs(string, reemplazo);
                break;

              case STATUS:
                replace(string, data[COLUMNAS-2], new);
                fputs(string, reemplazo);
                break;

              case PASSWD:
                replace(string, data[COLUMNAS-3], new);
                fputs(string, reemplazo);
                break;
            }
        }
      else fputs(string, reemplazo);
    }

  fclose(archivo);
  fclose(reemplazo);

  remove(DB_PATH);
  rename(TMP_PATH, DB_PATH);
  return 1;
}

/**
 * @brief
 * Función encargada de reemplazar en una línea los campos que se pasen como
 * parámetros.
 * @param str Linea que contiene los datos del usuario.
 * @param old Valor a reemplazar.
 * @param new Nuevo valor.
 */
void replace(char *str, const char *old, const char *new)
{
  char *pos, temp[LINE_LEN];
  long indice = 0;
  size_t len;

  len = strlen(old);

  while((pos = strstr(str,old)) != NULL)
    {
      strcpy(temp,str); //backup de la línea

      indice = pos - str; //posicion en la que se encontró la palabra

      str[indice] = '\0'; //termina la línea en esa posición

      strcat(str,new); //concatena con el contendio nuevo

      strcat(str, temp+indice+len); //concatena con el resto
    }
}

/**
 * @brief
 * Función encargada de obtener la fecha actual.
 * @param date[20] Fecha actual.
 */
void get_date(char date[20])
{
  int dd, mm, yyyy, h, min, seg;

  time_t now;

  // Obtain current time
  // time() returns the current time of the system as a time_t value
  time(&now);

  // localtime converts a time_t value to calendar time and
  // returns a pointer to a tm structure with its members
  // filled with the corresponding values
  struct tm *local = localtime(&now);

  h   = local->tm_hour;
  min = local->tm_min;
  seg = local->tm_sec;

  dd   = local->tm_mday;        	// get day of month (1 to 31)
  mm   = local->tm_mon + 1;   	// get month of year (0 to 11)
  yyyy = local->tm_year + 1900;	// get year since 1900

  sprintf(date,"%02d/%02d/%d %02d:%02d:%02d\n", dd, mm, yyyy, h, min, seg);
  date[strlen(date)-1] = '\0';
}
