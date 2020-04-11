#include <stdint.h>

#define PORT_PS 4444  /**< Puerto para 'primary_server'. */
#define PORT_FI 5555  /**< Puerto para 'files_service'. */
#define MAX     80    /**< Tamaño del buffer. */

/**
 * Escribe en el socket deseado un mensaje.
 * @param  sockfd Socket en el que se desea escribir el mensaje.
 * @param  cmd    Comando enviado por el usuario.
 * @return        Si el write fue exitoso.
 */
ssize_t send_cmd(int sockfd, void *cmd);

/**
 * Lee lo que se encuentra en el socket.
 * @param  sockfd Socket del que se desea leer el mensaje.
 * @param  cmd    Comando enviado por el usuario.
 * @return        Si el read fue exitoso.
 */
ssize_t recv_cmd(int sockfd, void *cmd);

/**
 * Función encargada de crear el socket en el puerto pedido.
 * @param  port Puerto al que se desea conectar.
 s* @return      File descriptor del socket creado.
 */
int create_svsocket(uint16_t port);

/**
 * Función encargada de conectar al cliente en el puerto del servidor.
 * @param  port Puerto al que se desea conectar.
 * @return      File descriptor del socket conectado.
 */
int create_clsocket(uint16_t port);
