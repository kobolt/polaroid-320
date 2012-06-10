#ifndef _COMM_H
#define _COMM_H

#include <stdlib.h> /* size_t */

int comm_command(int tty, unsigned char command, unsigned char argument,
  int (*response_callback)(char *, size_t));
void comm_get_picture_data(int tty, char picture_no, long size, 
  unsigned char *out);

#endif /* _COMM_H */
