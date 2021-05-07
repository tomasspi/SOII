/**
 * @file main.c
 * @brief
 * Aplica ambos filtros a la imagen BMP.
 *
 * @author Tomás Santiago Piñero
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <omp.h>

#include "simple_bmp.h"

#define SIZE_K  13
#define BUF_LEN 1024
#define K       1.8
#define L       15

#define PATH    "../imgs/"
#define OUTPUT  "../imgs/filtered_image.bmp"

sbmp_image in, out;

typedef struct center
{
  int32_t pos_x;
  int32_t pos_y;
} img_center;

void            apply_filters(img_center c, int r, int threads);

sbmp_raw_data   blur_filter(int x, int y, uint16_t **kernel, uint16_t norm);

sbmp_image      create_image(int32_t ancho, int32_t alto);

uint16_t        get_norm(uint16_t **kernel, int16_t size);

void            kernel_setup(uint16_t **kern, int16_t ksize);

sbmp_raw_data   lineal_filter(sbmp_raw_data pixel);

sbmp_image      load_image(char name[BUF_LEN]);

inline int32_t  normalize_pixel_color(int32_t color);

void            save_image(char path[BUF_LEN]);

int main(int argc, char *argv[])
{
  if(argc < 4 || argc > 4)
    {
      fprintf(stderr, "Uso: %s `nombre_imagen' `radio' `cantidad_hilos'\n", argv[0]);
      exit(EXIT_FAILURE);
    }

  int r, threads;
  sscanf(argv[2], "%d", &r);
  sscanf(argv[3], "%d", &threads);

  in = load_image(argv[1]);

  img_center centro;
  centro.pos_x = in.info.image_width  / 2;
  centro.pos_y = in.info.image_height / 2;

  out = create_image(in.info.image_width, in.info.image_height);

  apply_filters(centro, r, threads);

  save_image(OUTPUT);

  printf("------------------------------------------------\n");

  return EXIT_SUCCESS;
}

/**
 * @brief
 * Obtiene la suma de los valores de la matriz kernel para normalizar el
 * resultado de la operación.
 *
 * @param  kernel Matriz del kernel.
 * @param  size   Tamaño de la matriz.
 * @return        Suma de los elementos de la matriz de kernel.
 */
uint16_t get_norm(uint16_t **kernel, int16_t size)
{
  uint16_t sum = 0;
  for (int i = 0; i < size; i++)
    {
      for (int j = 0; j < size; j++)
        sum = (uint16_t) (sum + kernel[i][j]);
    }
  return sum;
}

/**
 * @brief
 * Crea una nueva imagen vacía. Esta imagen va a tener los píxeles modificados
 * de la imagen de entrada.
 *
 * @param  out   Imagen de salida
 * @param  ancho Ancho de la imagen de salida
 * @param  alto  Alto de la imagen de salida
 * @return       Imagen de salida
 */
sbmp_image create_image(int32_t ancho, int32_t alto)
{
  int32_t result;
  result = sbmp_initialize_bmp(&out, (uint32_t) alto, (uint32_t) ancho);

  if(result == SBMP_OK);
    // printf("Nueva imagen 'filtered_image.bmp' inicializada.\n");
  else
    {
      perror("Crear imagen");
      exit(EXIT_FAILURE);
    }

  return out;
}

/**
 * @brief
 * Guarda la imagen BMP con los filtros aplicados.
 * @param path Path de la imagen de salida.
 * @param out  Imagen a guardar.
 */
void save_image(char path[BUF_LEN])
{
  int result = sbmp_save_bmp(path, &out);
  sync();

  if(result == SBMP_OK);
    // printf("Imagen 'filtered_image.bmp' guardada.\n");
  else
    {
      perror("Guardar imagen");
      exit(EXIT_FAILURE);
    }
}

/**
 * @brief
 * Carga una imagen BMP a la estructura.
 * @param  image Imagen BMP a cargar.
 * @param  name  Nombre de la imagen.
 * @return       Imagen.
 */
sbmp_image load_image(char name[BUF_LEN])
{
  char path[BUF_LEN];
  strcpy(path, PATH);
  strcat(path, name);

  int32_t result;
  result = sbmp_load_bmp(path, &in);

  if(result == SBMP_OK)
    {
        ;
      // printf("Imagen '%s' cargada.\n", name);
      // printf(" Ancho: %d [px]\n Alto:  %d [px]\n",
      //        in.info.image_width, in.info.image_height);
    }
  else
    {
      perror("Cargar imagen");
      exit(EXIT_FAILURE);
    }

  return in;
}


/**
 * @brief
 * Aplica el filtro lineal y 'blur' a la imagen.
 * @param in  Imagen BMP de entrada.
 * @param out Imagen BMP de salida.
 * @param c   Centro de la imagen.
 * @param r   Radio que define dónde se aplica cada filtro.
 */
void apply_filters(img_center c, int r, int threads)
{
  int32_t distancia = 0;
  int32_t distancia_y = 0;
  int32_t radio_cuadrado = r*r;

  uint16_t **kernel = calloc(SIZE_K, sizeof (int *));

  for(int k = 0; k < SIZE_K; k++)
    kernel[k] = calloc(SIZE_K, sizeof (uint16_t));

  kernel_setup(kernel, SIZE_K);

  uint16_t norm = get_norm(kernel, SIZE_K);
  int32_t offset = (int32_t) ((SIZE_K-1) / 2);

  double start = omp_get_wtime();

  #pragma omp parallel num_threads(threads)
  #pragma omp for schedule(dynamic)
  for (int32_t y = offset; y < in.info.image_height-offset; y++)
  {
    distancia_y = (y - c.pos_y)*(y - c.pos_y);

    for (int32_t x = offset; x < in.info.image_width-offset; x++)
    {
      distancia = (x - c.pos_x)*(x - c.pos_x) + distancia_y;

      if(distancia <= radio_cuadrado) /** Inside circle */
        out.data[y][x] = lineal_filter(in.data[y][x]);
      else /* Outside circle */
        out.data[y][x] = blur_filter(x, y, kernel, norm);
    }
  }

  printf("\n-- Tiempo transcurrido (%d hilos): %f --\n\n", threads,
         omp_get_wtime() - start);
}

/**
 * @brief
 * Realiza la convolución bidimensional con la imagen y el kernel.
 *
 * @param  in     Imagen de entrada.
 * @param  x      Posición 'x' del pixel.
 * @param  y      Posición 'y' del pixel.
 * @param  kernel Matriz del filtro 'blur'.
 * @param  norm   Valor para normalizar el resultado.
 * @return        Pixel con el filtro aplicado.
 */
sbmp_raw_data blur_filter(int x, int y, uint16_t **kernel, uint16_t norm)
{
  int32_t sum_r = 0;
  int32_t sum_g = 0;
  int32_t sum_b = 0;
  int32_t offset = (int32_t) ((SIZE_K-1) / 2);

  for (int32_t m = -offset; m <= offset; m++)
    {
      for (int32_t n = -offset; n <= offset; n++)
        {
          sum_r += (int32_t) (kernel[m+offset][n+offset] * in.data[y+m][x+n].red);
          sum_g += (int32_t) (kernel[m+offset][n+offset] * in.data[y+m][x+n].green);
          sum_b += (int32_t) (kernel[m+offset][n+offset] * in.data[y+m][x+n].blue);
        }
    }
  sum_r = (int32_t) (sum_r / norm);
  sum_g = (int32_t) (sum_g / norm);
  sum_b = (int32_t) (sum_b / norm);

  return (sbmp_raw_data) {(uint8_t) sum_b, (uint8_t) sum_g, (uint8_t) sum_r};
}

/**
 * @brief
 * Función encargada de aplicar a un pixel el filtro lineal.
 * @param  pixel Pixel a modificar.
 * @return       Pixel modificado.
 */
sbmp_raw_data lineal_filter(sbmp_raw_data pixel)
{
  int32_t r, g, b;
  r = pixel.red;
  g = pixel.green;
  b = pixel.blue;

  r = (int32_t) (r * K) + L;
  r = normalize_pixel_color(r);

  g = (int32_t) (g * K) + L;
  g = normalize_pixel_color(g);

  b = (int32_t) (b * K) + L;
  b = normalize_pixel_color(b);

  pixel = (sbmp_raw_data) { (uint8_t) b, (uint8_t) g, (uint8_t) r};

  return pixel;
}

/**
 * @brief
 * Normaliza el color del pixel (el valor puede ir de 0 a 255).
 * @param  color Color a normalizar.
 * @return       Color normalizado.
 */
inline int32_t normalize_pixel_color(int32_t color)
{
  if(color > 255)
    color = 255;
  else if(color < 0)
    color = 0;

  return color;
}

/**
 * @brief
 * Inicializa la matriz para el filtro 'blur'.
 *
 * @param kern  Matriz.
 * @param ksize Tamaño de la matriz.
 */
void kernel_setup(uint16_t **kern, int16_t ksize)
{
  uint16_t st_val = 1;
  for (int j = 0; j < ksize; j++)
    kern[0][j] = st_val;

  for (int i = 1; i < ksize / 2 + 1; i++)
  {
    for (int j = 0; j < ksize; j++)
    {
      if (j >= i && j < (ksize - i))
        kern[i][j] = (uint16_t)(kern[i - 1][j] + (uint16_t)1);
      else
        kern[i][j] = kern[i - 1][j];
    }
  }

  for (int i = ksize / 2; i < ksize; i++)
  {
    for (int j = 0; j < ksize; j++)
      kern[i][j] = kern[ksize - i - 1][j];
  }
}
