#ifndef _JPEG_H
#define _JPEG_H

void jpeg_decode(int (next_byte(void)),
  void (process_block(int block[], int block_no)), int yq, int cbq, int crq);

#endif /* _JPEG_H */
