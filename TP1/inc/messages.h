/**
 * @file messages.h
 * @brief
 * Header para los mensajes de SystemV msgqueues.
 *
 * @author Tomás Santiago Piñero
 */
#define STR_LEN  1024       /**< Largo de los strings */
#define QU_PATH  "./server" /**< Archivo para crear la cola de mensajes. */

/**
 * @brief
 * Enum con los destinos de los mensajes a enviar en la cola de mensajes.
 */
enum msg_ID
{
  to_auth = 1, /**< Destinado a 'auth_service'. */
  to_file = 2, /**< Destinado a 'files_service'. */
  to_prim = 3  /**< Destinado a 'primary_server'. */
};

/**
 * @brief
 * Enum con los estados posibles del usuario.
 */
enum status
{
  blocked = 1, /**< El usuario está bloqueado. */
  active  = 2, /**< El usuario está activo. */
  invalid = 3, /**< Credenciales inválidas. */
  not_reg = 4  /**< Es usuario no existe. */
};

struct msg            /** Estructura para mensajes generales. */
{
  long mtype;         /**< Identificador del mensaje. */
  char msg[STR_LEN];  /**< Contenido del mensaje. */
};
