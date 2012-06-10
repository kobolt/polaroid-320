#include "comm.h"
#include "jpeg.h"
#include "pnm.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <arpa/inet.h> /* ntohs() */

#define DEFAULT_DEVICE "/dev/ttyS0" /* Common first serial device in Linux. */



typedef enum {
  OUTPUT_NONE,
  OUTPUT_COLOR,
  OUTPUT_GREY,
  OUTPUT_RAW,
  OUTPUT_NODEC,
  OUTPUT_ERASE,
} output_type_t;



/* Data shared between callback functions and the main loop. */
static unsigned char *picture_data;
static int picture_data_size;
static int picture_data_count;
static FILE *output_file;



static void display_help(void)
{
  fprintf(stderr, "\nOptions:\n"
    "  -h          Display this help and exit.\n"
    "  -e          Erase/delete all pictures.\n"
    "  -d DEVICE   Use DEVICE instead of %s.\n"
    "  -c          Color output (default) (PPM format).\n"
    "  -g          Greyscale output (luminance only) (PGM format).\n"
    "  -r          Raw component output (no quantization) (PGM format).\n"
    "  -n          No JPEG decoding (dump raw picture data).\n\n",
     DEFAULT_DEVICE);
}



static int parse_camera_info(char *buffer, size_t buffer_size)
{
  int i;

  if (buffer_size != 14)
    error(1, 0, "%s.%d: Invalid camera info buffer size: %d",
      __FILE__, __LINE__, buffer_size);

  if (buffer[0] != 0x00)
    error(0, 0, "%s.%d: Wrong camera info header: 0x%02X",
      __FILE__, __LINE__, buffer[0]);

  printf("Camera information: ");
  for (i = 1; i < buffer_size; i++) {
    if (isprint(buffer[i]))
      printf("%c", buffer[i]);
    else
      printf(" (0x%02X)", (unsigned char)buffer[i]);
  }
  printf("\n");

  return 0;
}



static int parse_camera_state(char *buffer, size_t buffer_size)
{
  if (buffer_size != 24)
    error(1, 0, "%s.%d: Invalid camera state buffer size: %d",
      __FILE__, __LINE__, buffer_size);

  if (buffer[0] != 0x02)
    error(0, 0, "%s.%d: Wrong camera state header: 0x%02X",
      __FILE__, __LINE__, buffer[0]);

  /*
  printf("Horizontal pixels : %hu\n", ntohs(*(short int *)&buffer[2]));
  printf("Vertical pixels   : %hu\n", ntohs(*(short int *)&buffer[4]));
  */

  return 0;
}



static int parse_no_of_pictures(char *buffer, size_t buffer_size)
{
  if (buffer[0] != 0x03)
    error(0, 0, "%s.%d: Wrong number of pictures header: 0x%02X",
      __FILE__, __LINE__, buffer[0]);

  if (buffer_size > 1) {
    printf("Pictures on camera: %d\n", (int)buffer[1]);
    return buffer[1];
  } else
    error(1, 0, "%s.%d: Number of pictures buffer too small.",
      __FILE__, __LINE__);

  return 0;
}



static int parse_picture_size(char *buffer, size_t buffer_size)
{
  long int size;

  if (buffer_size != 7)
    error(1, 0, "%s.%d: Invalid picture size buffer size: %d",
      __FILE__, __LINE__, buffer_size);

  if (buffer[0] != 0x06)
    error(0, 0, "%s.%d: Wrong picture size header: 0x%02X",
      __FILE__, __LINE__, buffer[0]);

  size = ntohl(*(long int *)&buffer[1]);
  
  return size;
}



static int read_picture_data(void)
{
  if (picture_data_count >= picture_data_size)
    return EOF;
  else
    return *(picture_data + picture_data_count++);
}



/* Open a new file without overwriting an old one. */
static FILE *open_exclusive_file(int picture_no, char *extension)
{
  int try;
  char name[32];
  FILE *fh;

  for (try = 0; ; try++) {
    if (try == 0)
      snprintf(name, sizeof(name), "polaroid.%02d.%s",
        picture_no, extension);
    else
      snprintf(name, sizeof(name), "polaroid.%02d.%s.%d",
        picture_no, extension, try);

    /* Note: 'x' is a GNU C library extension. */
    fh = fopen(name, "wx");
    if (fh == NULL) {
      if (errno != EEXIST)
        error(1, errno, "%s.%d: fopen()", __FILE__, __LINE__);
    } else
      break; /* Opened an exclusive file. */

    /* File already exists, try another one. */
  }
  
  return fh;
}



int main(int argc, char *argv[])
{
  int i, j, c, tty, no_of_pictures;
  struct termios tty_settings;
  char *device = NULL;
  char *component_ext[4] = {"y1.pgm", "cb.pgm", "cr.pgm", "y2.pgm"};
  output_type_t output_type = OUTPUT_NONE;

  while ((c = getopt(argc, argv, "hed:cgrn")) != -1) {
    switch (c) {
    case 'h':
      display_help();
      return 0;

    case 'd':
      device = optarg;
      break;

    case 'c':
    case 'g':
    case 'r':
    case 'n':
    case 'e':
      if (output_type != OUTPUT_NONE) {
        error(1, 0,
          "%s.%d: Only one of the options -c, -g, -r, -n or -e can be set.",
          __FILE__, __LINE__);
      } else {
        if (c == 'c')
          output_type = OUTPUT_COLOR;
        else if (c == 'g')
          output_type = OUTPUT_GREY;
        else if (c == 'r')
          output_type = OUTPUT_RAW;
        else if (c == 'n')
          output_type = OUTPUT_NODEC;
        else if (c == 'e')
          output_type = OUTPUT_ERASE;
      }
      break;

    case '?':
    default:
      display_help();
      return 1;
    }
  }

  if (output_type == OUTPUT_NONE)
    output_type = OUTPUT_COLOR; /* The default choice. */

  if (device == NULL)
    device = DEFAULT_DEVICE;
      
  tty = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (tty == -1)
    error(1, errno, "%s.%d: open(): %s: ", __FILE__, __LINE__, device);

  if (tcgetattr(tty, &tty_settings) == -1)
    error(1, errno, "%s.%d: tcgetattr()", __FILE__, __LINE__);

  cfmakeraw(&tty_settings);
  cfsetispeed(&tty_settings, B115200);
  cfsetospeed(&tty_settings, B115200);

  if (tcsetattr(tty, TCSANOW, &tty_settings) == -1)
    error(1, errno, "%s.%d: tcsetattr()", __FILE__, __LINE__);

  /* Initialize camera. */
  comm_command(tty, 0x00, 0, NULL);
  comm_command(tty, 0x01, 0, parse_camera_info);
  comm_command(tty, 0x02, 0, parse_camera_state);
  comm_command(tty, 0x0A, 0, NULL);

  if (output_type == OUTPUT_ERASE) {
    comm_command(tty, 0x07, 0, NULL); /* Delete all pictures. */
    printf("--- ALL PICTURES ERASED ---\n");
    close(tty);
    return 0;
  }

  /* Get amount of pictures and loop for each picture. */
  no_of_pictures = comm_command(tty, 0x03, 0, parse_no_of_pictures);
  printf("----------------------------------------"
         "----------------------------------------\n");
  for (i = 1; i <= no_of_pictures; i++) {
    picture_data_size = comm_command(tty, 0x04, i, parse_picture_size);
    picture_data = (unsigned char *)malloc(sizeof(unsigned char) *
      picture_data_size);

    /* Skip 6 first bytes when starting the read. */
    /* This is some fake header, and not valid JPEG data. */
    picture_data_count = 6;

    comm_get_picture_data(tty, i, picture_data_size, picture_data);

    switch (output_type) {
    case OUTPUT_COLOR:
      output_file = open_exclusive_file(i, "ppm");
      /* PPM header, dimensions and max-val. */
      pnm_init(output_file, 0, "P3\n320 240\n255\n");
      /* Quantization value 4 for luminance and 2 for each chrominace
         component seems to produce the best overall result for all pictures.
         Note: The colors will be a bit pale. */
      jpeg_decode(read_picture_data, pnm_block_to_ppm, 4, 2, 2);
      break;

    case OUTPUT_GREY:
      output_file = open_exclusive_file(i, "pgm");
      /* PGM header, dimensions and max-val. */
      pnm_init(output_file, 0, "P2\n160 120\n255\n");
      /* Lumiance quantzation of 4 seems to be about right. */
      jpeg_decode(read_picture_data, pnm_component_to_pgm, 4, 0, 0);
      break;

    case OUTPUT_RAW:
      for (j = 0; j < 4; j++) {
        picture_data_count = 6; /* Need to reset for each round... */
        output_file = open_exclusive_file(i, component_ext[j]);
        pnm_init(output_file, j, "P2\n160 120\n255\n");
        /* No quantization for the components, needs to be handled by
           an external tool later. */
        jpeg_decode(read_picture_data, pnm_component_to_pgm, 1, 1, 1);
        if (j < 3)
          fclose(output_file);
      }
      break;

    case OUTPUT_NODEC:
      output_file = open_exclusive_file(i, "dat");
      fwrite(picture_data, sizeof(char), picture_data_size, output_file);
      break;

    default:
      break;
    }

    fclose(output_file);
    free(picture_data);
  }

  close(tty);
  return 0;
}

