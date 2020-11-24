/**
 * @file status_service.c
 * @brief
 * Servicio que devuelve las especificaciones del servidor.
 *
 * @author Tomás Santiago Piñero
 */

#include <ulfius.h>
#include <jansson.h>
#include <syslog.h>

#include "utilities.h"

#define PORT 8081

typedef struct uptime   /** Estructura para obtener el uptime. */
{
  int time; /**< Uptime total en segundos. */
  int h;    /**< Horas de uptime. */
  int min;  /**< Minutos de uptime. */
  int seg;  /**< Segundos de uptime. */
} upinfo;

typedef struct hardinfo /** Estructura para obtener la información del hardware. */
{
  char *kernelVersion; /**< Versión de kernel. */
  char *processorName; /**< Procesador. */
  int totalCPUCore;    /**< Cantidad de núcleos del procesador. */
  int totalMemory;     /**< Cantidad de memoria RAM total en MB. */
  int freeMemory;      /**< Cantidad de memoria RAM libre en MB. */
  int diskTotal;       /**< Capacidad del disco en GB. */
  int diskFree;        /**< Espacio libre del disco en GB. */
  float loadAvg;       /**< Carga promedio del sistema en 1 minuto. */
  upinfo uptime;       /**< Cuánto tiempo hace que se inició el sistema. */
} hwinfo;

void 			   get_hwinfo(hwinfo *info);
inline char *remove_spaces(char *tmp);

/**
 * Callback function for the web application on /helloworld url call
 */
int callback_status(const struct _u_request * request,
                    struct _u_response * response, void * user_data)
{
  printf("[%s]\n", request->http_verb);
  if(user_data == NULL){};

  const char *client;
  client = u_map_get(request->map_header, "X-Real-IP");

  json_t *body;

  hwinfo info;
  get_hwinfo(&info);

  char up[BUFF];
  sprintf(up, "%dh %02dm %02ds", info.uptime.h, info.uptime.min, info.uptime.seg);

  body = json_pack("{s:s, s:s, s:i, s:i, s:i, s:i, s:i, s:f, s:s}",
                   "kernelVersion", info.kernelVersion,
                   "processorName", info.processorName,
                   "totalCPUCore", info.totalCPUCore,
                   "totalMemory", info.totalMemory,
                   "freeMemory", info.freeMemory,
                   "diskTotal", info.diskTotal,
                   "diskFree", info.diskFree,
                   "loadAvg", info.loadAvg,
                   "uptime", up);

  syslog(LOG_NOTICE, "Estadísicas requeridas desde %s\n", client);
  char msg[BUFF];
  sprintf(msg, "Estadísicas requeridas desde %s\n", client);
  write_log(get_date(), STAT, msg);

  ulfius_set_json_body_response(response, 200, body);
  return U_CALLBACK_CONTINUE;
}

/**
 * main function
 */
int main(void)
{
  struct _u_instance instance;

  // Initialize instance with the port number
  if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK)
    {
      syslog(LOG_ERR, "Error en ulfius_init_instance.\n");
      return(EXIT_FAILURE);
    }

  // Endpoint list declaration
  ulfius_add_endpoint_by_val(&instance, "GET", "/hardwareinfo",
                             NULL, 0, &callback_status, NULL);

  // Start the framework
  if (ulfius_start_framework(&instance) == U_OK)
    {
      syslog(LOG_INFO, "Framework iniciado en puerto %d\n", instance.port);
      printf("Framework iniciado en puerto %d\n", instance.port);
      pause();
    }
  else
    {
      syslog(LOG_ERR, "Error al iniciar framework.\n");
      printf("Error al iniciar framework.\n");
    }

  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);

  return EXIT_SUCCESS;
}

/**
 * @brief
 * Obtiene la información del sistema y la guarda en la estructura pasada como
 * argumento.
 * @param hard Estructura que almacena la información.
 */
void get_hwinfo(hwinfo *hard)
{
  hard->kernelVersion = get_popen("uname -r");

  char *tmp = get_popen("cat /proc/loadavg");
  sscanf(tmp, "%f", &hard->loadAvg);

  tmp = get_popen("cat /proc/uptime");
  sscanf(tmp, "%d", &hard->uptime.time);

  hard->uptime.h   = hard->uptime.time/60/60%24;
  hard->uptime.min = hard->uptime.time/60%60;
  hard->uptime.seg = hard->uptime.time%60;

  tmp = get_popen("free --mega | grep Mem");
  sscanf(tmp, "%*s %d %*d %d", &hard->totalMemory, &hard->freeMemory);

  tmp = get_popen("lscpu | grep name");
  tmp = remove_spaces(tmp);
  hard->processorName = tmp;

  tmp = get_popen("lscpu | grep socket");
  tmp = remove_spaces(tmp);
  sscanf(tmp, "%d", &hard->totalCPUCore);

  tmp = get_popen("df . | grep /");
  sscanf(tmp, "%*s %d %*d %d", &hard->diskTotal, &hard->diskFree);
  hard->diskTotal /= 1024000;
  hard->diskFree /= 1024000;

  free(tmp);
}

/**
 * @brief
 * Detecta los ':' de la string y elimina los espacios del
 * resultado del comando 'lscpu'.
 * @param  tmp Línea que contiene los datos.
 * @return tmp Línea con los datos necesarios.
 */
inline char *remove_spaces(char *tmp)
{
  char *p = strchr(tmp,':');
  if(p != NULL)
    {
    	  memmove(tmp, tmp+(p-tmp+1), strlen(tmp));
    	  while(*tmp == ' ')
      	  {
      	  	if(*tmp != ' ')
      	  		break;
      	  	else
      	  		++tmp;
      	  }
    }
  return tmp;
}
