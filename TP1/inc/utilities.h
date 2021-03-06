/**
 * @file utilities.h
 * @brief
 * Header con las utilidades a los procesos.
 *
 * @author Tomás Santiago Piñero
 */

#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define MAX  256       /**< Largo del path */
/**
 * @brief
 * Función encargada de checkear el valor de operaciones que devuelvan -1 cuando
 * ocurre un error.
 *
 * @param err Número.
 */
void check_error(int err);

/**
 * @brief
 * Función encargada de obtener la cola de mensajes.
 * @return msqid ID de la cola de mensajes.
 */
int get_queue();

/**
 * @brief
 * Función encargada de calcular el hash MD5 de un archivo. Si 'size' es 0,
 * entonces se leerá todo el archivo.
 * @param  path Path del USB.
 * @param  size Tamaño de cálculo.
 * @return      Hash MD5 del archivo.
 */
char *get_md5(char path[MAX], size_t size);

/**
 * @brief
 * Indica si el mensaje enviado son credenciales de login o un comando.
 * @param  msg Mensaje recibido.
 * @return     Si son credenciales o no.
 */
bool is_login(char msg[MAX]);
