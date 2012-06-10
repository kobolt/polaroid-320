#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>



#ifdef COMM_DEBUG
static void dump_hex(char *buffer, size_t buffer_size)
{
  int n = 0;
  while (n < buffer_size)
    fprintf(stderr, " %02X", (unsigned char)buffer[n++]);
}
#endif



int comm_command(int tty, unsigned char command, unsigned char argument,
  int (*response_callback)(char *, size_t))
{
  char cmd[16], response[64];
  int cmd_size, response_size, read_timeout = 0;

  if (argument > 0) {
    snprintf(cmd, sizeof(cmd), "\xE6\xE6\xE6\xE6" "%c" "%c" "%c" "%c",
      command, argument, command ^ 0xFF, argument ^ 0xFF);
    cmd_size = 8;
  } else if (command > 0) {
    snprintf(cmd, sizeof(cmd), "\xE6\xE6\xE6\xE6" "%c" "%c" "\xFF",
      command, command ^ 0xFF);
    cmd_size = 7;
  } else { /* Special init command. */
    memcpy(cmd, "\xE6\xE6\xE6\xE6\xE6\xE6\xE6\xE6\x00\xFF\xFF", 11);
    cmd_size = 11;
  }

#ifdef COMM_DEBUG
  fprintf(stderr, ">");
  dump_hex(cmd, cmd_size);
  fprintf(stderr, "\n");
#endif

  if (write(tty, cmd, cmd_size) == -1)
    error(1, errno, "%s.%d: write()", __FILE__, __LINE__);

  usleep(10000);

read_again:
  response_size = read(tty, response, sizeof(response));

  if (response_size == -1) {
    if (errno == EAGAIN) {
      if (read_timeout)
        error(1, 0, "%s.%d: Camera not responding.", __FILE__, __LINE__);
      else {
        read_timeout = 1;
        sleep(3);
        goto read_again;
      }

    } else
      error(1, errno, "%s.%d: read()", __FILE__, __LINE__);
  }

#ifdef COMM_DEBUG
  fprintf(stderr, "<");
  dump_hex(response, response_size);
  fprintf(stderr, "\n");
#endif

  /* Note: Checksum is not verified. A possible improvement? */

  if (response_callback != NULL)
    return response_callback(response, response_size);
  
  return 0;
}



static void progress_bar(long total, long remaining)
{
  int i;

  printf("\r|");
  for (i = 0; i < 60; i++) {
    if (i * (total / 60) > total - remaining)
      printf(" ");
    else
      printf("#");
  }
  printf("| %lu/%lu", total - remaining, total);

  if (remaining == 0)
    printf("\n");
}



void comm_get_picture_data(int tty, char picture_no, long size,
  unsigned char *out)
{
  char buffer[2048]; /* Camera internal buffer reported to be 2000 bytes. */
  int buffer_size, limit, data_read, data_written;
  long original_size = size;

  snprintf(buffer, sizeof(buffer), "\xE6\xE6\xE6\xE6\x05" "%c" "\xFA" "%c",
    picture_no, picture_no ^ 0xFF);
  buffer_size = 8;

#ifdef COMM_DEBUG
  fprintf(stderr, ">");
  dump_hex(buffer, buffer_size);
  fprintf(stderr, "\n");
#endif

  if (write(tty, buffer, buffer_size) == -1)
    error(1, errno, "%s.%d: write()", __FILE__, __LINE__);

  data_written = 0;
  while (size > 0) {
    usleep(10000);

    /* Read initial 5 byte frame header first. */
    data_read = read(tty, buffer, 5);
    if (data_read == -1)
      error(1, errno, "%s.%d: read()", __FILE__, __LINE__);

#ifdef COMM_DEBUG
    fprintf(stderr, "<");
    dump_hex(buffer, 5);
    fprintf(stderr, "\n");
#endif

    if (buffer[0] != 0x04)
      error(0, 0, "%s.%d: Wrong picture data frame header: 0x%02X",
        __FILE__, __LINE__, buffer[0]);

    if (size > 2000)
      limit = 2000;
    else
      limit = size;

    data_read = 0;
    while (limit > 0) {
      data_read = read(tty, buffer, limit);
      if (data_read == -1) {
        if (errno == EAGAIN) {
          usleep(1000);
          continue;
        } else
          error(1, errno, "%s.%d: read()", __FILE__, __LINE__);
      }
      limit -= data_read;
      size -= data_read;

#ifdef COMM_DEBUG
      fprintf(stderr, "<");
      dump_hex(buffer, data_read);
      fprintf(stderr, "\n");
#endif
      
      /* Write back data to caller's given memory location. */
      memcpy(out + data_written, buffer, data_read);
      data_written += data_read;
     
      progress_bar(original_size, size); 
      usleep(1000);
    }

    /* Read final 2 byte checksum. */
    data_read = read(tty, buffer, 2);
    if (data_read == -1)
      error(1, errno, "%s.%d: read()", __FILE__, __LINE__);

#ifdef COMM_DEBUG
    fprintf(stderr, "<");
    dump_hex(buffer, 2);
    fprintf(stderr, "\n");
#endif
  }

}

