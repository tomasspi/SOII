/**
 * @file utilities.c
 * @brief
 * Código fuente de 'utilities.h'
 *
 * @author Tomás Santiago Piñero
 */

#include "utilities.h"

/**
 * @brief
 * Escribe el log de los servicios.
 * @param timestamp Timestamp de la acción.
 * @param service   Servicio que escribe.
 * @param message   Mensaje a escribir.
 */
void write_log(char *timestamp, char *service, char *message)
{
  FILE *log;
  log = fopen(PATH, "a+");

  if (log == NULL)
    perror("fopen log");

  fprintf(log, "%s | %14s | %s", timestamp, service, message);
  fclose(log);
}

/**
 * @brief
 * Obtiene la hora y fecha actuales.
 * @return fecha y hora.
 */
char *get_date()
{
  char *date = malloc(50 * sizeof(char) + 1);

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

  dd   = local->tm_mday;        // get day of month (1 to 31)
  mm   = local->tm_mon + 1;   	// get month of year (0 to 11)
  yyyy = local->tm_year + 1900;	// get year since 1900

  sprintf(date,"%02d-%02d-%d %02d:%02d:%02d\n", dd, mm, yyyy, h, min, seg);
  date[strlen(date)-1] = '\0';

  return date;
}

/**
 * @brief
 * Realiza la ejecución del comando pasado como argumento.
 * @param  cmd  Comando a ejecutar.
 * @return data Resultado del comando ejecutado.
 */
char *get_popen(char *cmd)
{
  FILE *pf;
  char *data = malloc(BUFF * sizeof(char));

  pf = popen(cmd,"r");

  if(pf == NULL)
    perror("popen");

  if(fgets(data, BUFF, pf) == NULL)
    perror("fgets");

  data[strlen(data)-1] = '\0';

  if(pclose(pf) == -1)
    perror("pclose");

  return data;
}
