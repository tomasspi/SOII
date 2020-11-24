/**
 * @file users_service.c
 * @brief
 * Servicio que maneja los usuarios.
 *
 * @author Tomás Santiago Piñero
 */

#include <ulfius.h>
#include <jansson.h>
#include <sys/types.h>
#include <pwd.h>
#include <syslog.h>

#include "utilities.h"

#define PORT 8080

json_t *get_users();

/**
 * Callback function for the web application on /helloworld url call
 */
int callback_users_get(const struct _u_request * request,
                       struct _u_response * response, void * user_data)
{
  printf("[%s]\n", request->http_verb);
  if(user_data == NULL){};

  json_t *body = get_users();

  ulfius_set_json_body_response(response, 200, body);
  return U_CALLBACK_CONTINUE;
}

int callback_users_post(const struct _u_request * request,
                        struct _u_response * response, void * user_data)
{
  if(user_data == NULL){};

  json_t *json_request = ulfius_get_json_body_request(request, NULL);
  json_t *body;

  const char *user   = json_string_value(json_object_get(json_request, "username"));
  const char *passwd = json_string_value(json_object_get(json_request, "password"));

  if(user == NULL || passwd == NULL)
    {
      body = json_pack("{s:s}", "description", "Bad request");
      ulfius_set_json_body_response(response, 400, body);
      return U_CALLBACK_CONTINUE;
    }

  struct passwd *info = getpwnam(user);
  if(info != NULL)
    {
      body = json_pack("{s:s}", "description", "El usuario ya existe");
      ulfius_set_json_body_response(response, 409, body);
      return U_CALLBACK_CONTINUE;
    }

  char cmd[BUFF];
  sprintf(cmd,"sudo useradd %s -p $(openssl passwd -5 '%s')", user, passwd);

  char *tmp = get_popen(cmd);
  free(tmp);

  char *timestamp = get_date();

  struct passwd *passwd_info = getpwnam(user);

  body = json_pack("{s:i, s:s, s:s}", "id", passwd_info->pw_uid,
                   "username", user, "created_at", timestamp);

  ulfius_set_json_body_response(response, 200, body);

  syslog(LOG_NOTICE, "Usuario %d creado", passwd_info->pw_uid);

  char msg[BUFF];
  sprintf(msg,"Usuario %d creado\n", passwd_info->pw_uid);
  write_log(timestamp, USER, msg);
  free(timestamp);

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
  ulfius_add_endpoint_by_val(&instance, "GET", "/users", NULL, 0,
                             &callback_users_get, NULL);

  ulfius_add_endpoint_by_val(&instance, "POST", "/users", NULL, 0,
                             &callback_users_post, NULL);


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
 * Obtiene la lista de usuarios del SO con su id.
 * @return JSON con los datos.
 */
json_t *get_users()
{
  int users = 0;
  json_t *json_users_object = json_object();
  json_t *json_users_array  = json_array();

  json_object_set_new(json_users_object, "data", json_users_array);

  struct passwd *passwd_info = getpwent();

  while(passwd_info)
    {
      users++;
      json_t *json_array_data;
      json_array_data = json_pack("{s:i, s:s}", "user_id", passwd_info->pw_uid,
                                  "username", passwd_info->pw_name);

      json_array_append(json_users_array, json_array_data);

      passwd_info = getpwent();
    }

  endpwent();

  syslog(LOG_NOTICE, "Usuarios listados: %d\n", users);
  char msg[BUFF];
  sprintf(msg, "Usuarios listados: %d\n", users);
  write_log(get_date(), USER, msg);

  return json_users_object;
}
