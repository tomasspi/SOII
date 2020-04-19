/**
 * @file sockets.h
 *
 * @author Tomás Santiago Piñero
 */

#include <stdint.h>

enum ports        /**< Puertos para la conexión de los sockets. */
{
  port_ps = 4444, /**< Puerto para 'primary_server'. */
  port_fi = 5555  /**< Puerto para 'files_service'. */
};

#define STR_LEN 1024  /**< Largo de los strings */

#define SV_IP "127.0.0.1" /**< Dirección IP del servidor. */

/**
 * @brief
 * Escribe en el socket deseado un mensaje.
 * @param  sockfd Socket en el que se desea escribir el mensaje.
 * @param  cmd    Comando enviado por el usuario.
 * @return        Si el write fue exitoso.
 */
ssize_t send_cmd(int sockfd, void *cmd);

/**
 * @brief
 * Lee lo que se encuentra en el socket.
 * @param  sockfd Socket del que se desea leer el mensaje.
 * @param  cmd    Comando enviado por el usuario.
 * @return        Si el read fue exitoso.
 */
ssize_t recv_cmd(int sockfd, void *cmd);

/**
 * @brief
 * Función encargada de crear el socket en el puerto pedido.
 * @param  port Puerto al que se desea conectar.
 s* @return      File descriptor del socket creado.
 */
int create_svsocket(uint16_t port);

/**
 * @brief
 * Función encargada de conectar al cliente en el puerto del servidor.
 * @param  port Puerto al que se desea conectar.
 * @return      File descriptor del socket conectado.
 */
int create_clsocket(uint16_t port);
