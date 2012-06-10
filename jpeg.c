#include "huffman.h"
#include <stdlib.h>
#include <error.h>
#include <math.h>



/* Tables from the ISO/IEC 10918-1 : 1993(E) JPEG standard. */

/* Luminance DC coefficients. */
static unsigned char huffman_table_dc[] =
  "\x00\x01\x05\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b";

/* Luminance AC coefficients. */
static unsigned char huffman_table_ac[] =
  "\x00\x02\x01\x03\x03\x02\x04\x03\x05\x05\x04\x04\x00\x00\x01\x7d"
  "\x01\x02\x03\x00\x04\x11\x05\x12\x21\x31\x41\x06\x13\x51\x61\x07"
  "\x22\x71\x14\x32\x81\x91\xa1\x08\x23\x42\xb1\xc1\x15\x52\xd1\xf0"
  "\x24\x33\x62\x72\x82\x09\x0a\x16\x17\x18\x19\x1a\x25\x26\x27\x28"
  "\x29\x2a\x34\x35\x36\x37\x38\x39\x3a\x43\x44\x45\x46\x47\x48\x49"
  "\x4a\x53\x54\x55\x56\x57\x58\x59\x5a\x63\x64\x65\x66\x67\x68\x69"
  "\x6a\x73\x74\x75\x76\x77\x78\x79\x7a\x83\x84\x85\x86\x87\x88\x89"
  "\x8a\x92\x93\x94\x95\x96\x97\x98\x99\x9a\xa2\xa3\xa4\xa5\xa6\xa7"
  "\xa8\xa9\xaa\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xc2\xc3\xc4\xc5"
  "\xc6\xc7\xc8\xc9\xca\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xe1\xe2"
  "\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8"
  "\xf9\xfa";



static int power_of_two(int n)
{
  if (n == 0)
    return 1;
  else
    return 2 * power_of_two(n - 1);
}



static int next_bit(int (next_byte(void)))
{
  static int c, n = 0;

  if (n == 0) {
    c = next_byte();

    if (c == -1)
      return -1;

    if (c == 0xFF) { /* JPEG marker. */
      c = next_byte();
      if (c == 0x00)
        c = 0xFF; /* Just set back to 0xFF and continue. */
      else
        return -1; /* EOS (or some other) marker. */
    }
  }

  n--;
  if (n < 0)
    n = 7;

  if (c - power_of_two(n) >= 0) {
    c -= power_of_two(n);
    return 1;
  } else
    return 0;
}



static int receive(int (next_byte(void)), int category)
{
  int bit;
  int value = 0;
  
  if (category == 0)
    return 0;
    
  while ((bit = next_bit(next_byte)) != -1) {
    value += (bit * power_of_two(category - 1));
    category--;
    if (category <= 0)
      break; 
  }
 
  if (bit == -1)
    error(1, 0, "%s.%d: Unexpected EOF.", __FILE__, __LINE__);

  return value;
}



static int decode(int (next_byte(void)), huffman_t *table)
{
  int bit, value;
  while ((bit = next_bit(next_byte)) != -1) {
    table = huffman_lookup(table, bit, &value);
    if (table == NULL)
      return value;
  }
  return -1; /* EOF */
}



static int extend(int value, int category)
{
  if (category == 0)
    return value;
  if (value < power_of_two(category - 1))
    return value + (1 - power_of_two(category));
  else
    return value;
}



static void zig_zag_reorder(int block[])
{
  int n, x, y;
  int copy[64];

  for (n = 0; n < 64; n++)
    copy[n] = block[n];
  
  n = x = y = 0; 
  while (n < 63) {
    /* Step. */
    if (x < 7)
      x++;
    else
      y++;
    block[x + (y * 8)] = copy[++n];
    
    /* Down and left until limit. */
    while (x > 0 && y < 7) {
      x--;
      y++;
      block[x + (y * 8)] = copy[++n];
    }
    
    /* Step. */
    if (y < 7)
      y++;
    else
      x++;
    block[x + (y * 8)] = copy[++n];
    
    /* Up and right until limit. */
    while (y > 0 && x < 7) {
      y--;
      x++;
      block[x + (y * 8)] = copy[++n];
    }
  }
}



/* IDCT function loosely based on Tom Lane's public domain function. */
/* Note: Performance have been sacrificed for clarity. */
void idct(int block[])
{
  int x, y, u, v;
  double v_sum, u_sum;
  int result[64];

  for (x = 0; x < 8; x++) {
    for (y = 0; y < 8; y++) {

      v_sum = 0.0;
      for (v = 0; v < 8; v++) {

        u_sum = 0.0;
        for (u = 0; u < 8; u++) {
          u_sum += (double)block[v * 8 + u] * 0.5 *
            (cos((double)((x + x + 1) * u) * (M_PI / 16.0)) / 
            (double)((u == 0) ? sqrt(2.0) : 1.0));
        }

        v_sum += u_sum * 0.5 *
          (cos((double)((y + y + 1) * v) * (M_PI / 16.0)) / 
          (double)((v == 0) ? sqrt(2.0) : 1.0));
      }

      /* Round and store the result. */
      result[y * 8 + x] = (int)(v_sum < 0.0)
        ? - (0.5 - v_sum) : v_sum + 0.5;
    }
  }

  /* Update whole block with fresh results. */
  /* Needs to be done afterwards, as block is in use during calculations. */
  for (x = 0; x < 8; x++)
    for (y = 0; y < 8; y++)
      block[y * 8 + x] = result[y * 8 + x];
}



static int quantization_component(int block_no)
{
  /* Note: Tests have shown that both luminance components can use the same
     quantization, while the two chrominance components may require different
     quantization for each one. */
  switch (block_no % 4) {
  case 0:
  case 3:
    return 0;
  case 1:
    return 1;
  case 2:
    return 2;
  }
  return 0;
}



/* JPEG decoder loosely based on information from the official JPEG standard.
   Note: This decoder is fine-tuned against its special application and will
   voilate some of the rules specified in the official standard. */
/* next_byte() function should return 0-255 for a byte value or -1 on EOF.
   process_block() function assumes the caller understands what component
   is passed, based on the block number passed. */
void jpeg_decode(int (next_byte(void)),
  void (process_block(int block[], int block_no)), int yq, int cbq, int crq)
{
  huffman_t *dc, *ac;
  int i, n, category, zeroes, diff, block_no;
  int block[64];
  int prev_dc[4] = {0,0,0,0};

  /* Note: Only lumiance huffman tables are used, even for chrominance. */
  dc = huffman_convert_table(huffman_table_dc);
  ac = huffman_convert_table(huffman_table_ac);

  /* Loop for each 8x8 block. (64 byte vector.) */
  block_no = 0;
  while (1) {

    /* Decode DC coefficient. */
    category = decode(next_byte, dc);
    if (category == -1)
      break; /* EOF here is normal, just read the last block. */

    /* Note: Tests have shown that keeping the diff value for every fourth
       component produces the bext results. (1:1:1:1 sub-sampling?) */
    diff = receive(next_byte, category);
    diff = extend(diff, category);
    block[0] = prev_dc[block_no % 4] + diff;
    prev_dc[block_no % 4] = block[0];

    /* Decode AC coefficients. */
    n = 1;
    for (i = 1; i < 64; i++)
      block[i] = 0;
    while (n < 64) {
      category = decode(next_byte, ac);
      if (category == -1)
        error(1, 0, "%s.%d: Unexpected EOF.", __FILE__, __LINE__);
      zeroes   = category >> 4;  /* High nibble. */
      category = category & 0xF; /* Low nibble. */

      if (category == 0) {
        if (zeroes == 15)
          n += 16;
        else
          break;

      } else {
        n += zeroes;
        if (n >= 64)
          error(1, 0, "%s.%d: Buffer overflow.", __FILE__, __LINE__);
        block[n] = receive(next_byte, category);
        block[n] = extend(block[n], category);
        n++;
      }
    }

    /* Dequantize. */
    for (i = 0; i < 64; i++) {
      switch (quantization_component(block_no)) {
      case 0:
        block[i] *= yq;
        break;
      case 1:
        block[i] *= cbq;
        break;
      case 2:
        block[i] *= crq;
        break;
      }
    }

    /* Re-order vector back into 8x8 block from zig-zag ordering. */
    /* Note: This needs to be done after quantization it seems. */
    zig_zag_reorder(block);
    
    /* Inverse Discrete Cosine Transform. */
    idct(block);

    /* Level shift. */
    for (i = 0; i < 64; i++)
      block[i] += 128;

    /* Truncate out-of-range values created by IDCT. */
    /* Note: Only seems to be needed with high quantization values. */
    for (i = 0; i < 64; i++) {
      if (block[i] < 0)
        block[i] = 0;
      if (block[i] > 255)
        block[i] = 255;
    }

    /* Pass block back to caller for processing. */
    process_block(block, block_no);

    block_no++;
  }

  huffman_tree_free(dc);
  huffman_tree_free(ac);
}

