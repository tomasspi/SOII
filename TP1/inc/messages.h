/**
 * @file messages.h
 *
 * @author Tomás Santiago Piñero
 */

#define STR_LEN 1024 /**< Largo de los strings */

#define TO_AUTH 1    /**< Destinado a 'auth_service' */
#define TO_FILE 2    /**< Destinado a 'files_service' */
#define TO_PRIM 3    /**< Destinado a 'primary_server' */

#define BLOCKED 1    /**< El usuario está bloqueado. */
#define ACTIVE  2    /**< El usuario está activo. */
#define INVALID 3    /**< Credenciales inválidas. */
#define NOT_REG 4    /**< Es usuario no existe. */

#define QU_PATH  "./server" /**< Archivo para crear la cola de mensajes. */


struct msg            /** Estructura para mensajes generales. */
{
  long mtype;         /**< Identificador del mensaje: para auth o para files. */
  char msg[STR_LEN];  /**< Comando ingresado por el usuario. */
};
