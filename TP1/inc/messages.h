#define STR_LEN 1024 /**< Largo de los strings */
#define TO_AUTH 1    /**< Destinado a 'auth_service' */
#define TO_FILE 2    /**< Destinado a 'files_service' */
#define TO_PRIM 3    /**< Destinado a 'primary_server' */

#define BLOCKED 1    /**< El usuario está bloqueado. */
#define ACTIVE  2    /**< El usuario está activo. */
#define INVALID 3    /**< Credenciales inválidas. */
#define NOT_REG 4    /**< EL usuario no existe. */

/**< Estructura para enviar las credenciales de ingreso. */
struct log_msg
{
  long mtype;
  struct usr_cred
  {
    char username[10]; /**< Usuario ingresado. */
    char password[10]; /**< Password ingresado. */
  } login;
};

/**< Estructura para mensajes generales. */
struct msg
{
  long mtype;         /**< Identificador del mensaje: para auth o para files. */
  char msg[STR_LEN];  /**< Comando ingresado por el usuario. */
};
