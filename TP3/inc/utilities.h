/**
 * @file utilities.h
 * @brief
 * Header con funciones útiles a los servicios.
 *
 * @author Tomás Santiago Piñero
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include <unistd.h>

#define BUFF 1024                         /** Tamaño del buffer para strings. */
#define PATH "/var/log/tp3_services.log"  /** Ubicación del log. */
#define USER "user_service"               /** Servicio de usuarios. */
#define STAT "status_service"             /** Servicio de estado. */

/**
 * @brief
 * Obtiene la hora y fecha actuales.
 * @return fecha y hora.
 */
char *get_date();

/**
 * @brief
 * Realiza la ejecución del comando pasado como argumento.
 * @param  cmd  Comando a ejecutar.
 * @return data Resultado del comando ejecutado.
 */
char *get_popen(char *cmd);

/**
 * @brief
 * Escribe el log de los servicios.
 * @param timestamp Timestamp de la acción.
 * @param service   Servicio que escribe.
 * @param message   Mensaje a escribir.
 */
void write_log(char *timestamp, char *service, char *message);
