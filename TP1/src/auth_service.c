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
#define FECHA     'F'          /**< Utilizada para cambiar fecha en BD. */
#define PASSWD    'P'          /**< Utilizado para cambiar password en BD. */
#define LINE_LEN  50           /**< Largo de la linea a leer. */

char date[20]; /**< Fecha actual. */

void get_date();
void print_database();
void write_database(char data[COLUMNAS][STR_LEN], const char t, const char *new);
void reemplazar(char *str, const char *old, const char *new);
bool check_database(char input[2][STR_LEN], char database[COLUMNAS][STR_LEN]);
int check_credentials(char credentials[2][STR_LEN], char data[COLUMNAS][STR_LEN]);


int main(void)
{

  char input[2][STR_LEN];           /**< Almacena las credenciales recibidas. */
  char database[COLUMNAS][STR_LEN]; /**< Almacena los datos obtenidos de la BD. */

  get_date();
  printf("%s", date);

  struct log_msg log; /**< Mensaje de login. */
  struct msg buf;     /**< Mensaje de proósitos generales. */
  int msqid;
  key_t key;

  if ((key = ftok("../src/primary_server.c", 5)) == -1)
  {
    perror("ftok");
    exit(EXIT_FAILURE);
  }

  if ((msqid = msgget(key, 0666)) == -1)
  {
    perror("msgget");
    exit(EXIT_FAILURE);
  }

  buf.mtype = TO_PRIM;

  while (1)
  {
    msgrcv(msqid, &log, sizeof(log.login), TO_AUTH, 0);

    strcpy(input[0],log.login.username);
    strcpy(input[1],log.login.password);

    if(check_database(input, database))
    {
      switch(check_credentials(input, database))
      {
        case BLOCKED:
              printf("Blocked.\n");
              strcpy(buf.msg, "B");
              break;
        case ACTIVE:
              printf("Active.\n");
              strcpy(buf.msg, "A");
              break;
        case INVALID:
              printf("Invalid.\n");
              strcpy(buf.msg, "I");
              break;
      }
    } else
    {
      printf("Not registered.\n");
      strcpy(buf.msg,"NR");
    }

    msgsnd(msqid, &buf, sizeof(buf.msg), 0);

    break;
  }

  return EXIT_SUCCESS;
}

/**
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
  printf("\n");

  if(!strcmp(credentials[1],data[2]))
  {
    if(!strcmp(data[3],BLOCKED_Q))
      return BLOCKED;
    else
    {
      printf("Last login: %s\n", data[COLUMNAS-1]);

      if(data[COLUMNAS-1] != date)
      {
        get_date();
        write_database(data, FECHA, date);
      }
      return ACTIVE;
    }
  } else return INVALID;
}

/**
 * Función encargada de imprimir los datos que se encuentran registrados en la
 * base de datos (usuarios_database.txt).
 * @param archivo Archivo a imprimir.
 */
void print_database()
{
  char string[LINE_LEN];
  char *tmp, *contenido[COLUMNAS];
  int i;

  FILE *archivo = fopen(DB_PATH, "r");

  if(archivo == NULL)
    perror("Archivo DB");

  printf("============================================================\n");
  printf("%-15s %-15s %-15s\n", "Usuario", "Estado", "Último login");
  printf("============================================================\n");

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

    printf("%-15s %-15s %-15s\n", contenido[1], contenido[3], contenido[4]);
  }
  fclose(archivo);
}

/**
 * Función encargada de modificar los datos que se encuentran registrados en la
 * base de datos (usuarios_database.txt).
 * @param archivo                  Archivo a modificar.
 * @param data[COLUMNAS][STR_LEN]  Datos del usuario a modificar.
 * @param t                        Valor a modificar (FECHA o PASSWD).
 * @param new                      Nuevo valor.
 */
void write_database(char data[COLUMNAS][STR_LEN], const char t, const char *new)
{
  char string[LINE_LEN];
  FILE *archivo, *reemplazo;

  archivo = fopen(DB_PATH, "r");
  reemplazo = fopen("../res/DB.tmp", "w");

  if(archivo == NULL || reemplazo == NULL)
    perror("write_database");

  while(fgets(string, LINE_LEN, archivo) != NULL)
  {
    //Busco usuario por ID.
    if(!strncmp(string,data[0],1))
    {
      if(t == FECHA)
      {
        get_date();
        reemplazar(string, data[COLUMNAS-1], new);
        fputs(string, reemplazo);
      } else if (t == PASSWD)
      {
        reemplazar(string, data[COLUMNAS-3], new);
        fputs(string, reemplazo);
      }
    } else fputs(string, reemplazo);
  }
  fclose(archivo);
  fclose(reemplazo);

  remove(DB_PATH);
  rename("../res/DB.tmp", DB_PATH);
}

/**
 * Función encargada de reemplazar en una línea los campos que se pasen como
 * parámetros.
 * @param str Linea que contiene los datos del usuario.
 * @param old Valor a reemplazar.
 * @param new Nuevo valor.
 */
void reemplazar(char *str, const char *old, const char *new)
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

    str[strlen(str)-1] = '\0'; //quita el salto de linea molesto
  }
}

/**
 * Función encargada de obtener la fecha actual.
 */
void get_date()
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
}
