#include <stdio.h>

/* Portable aNyMap functions. */



/* 4 components, 20 blocks in width, 64 values per block. */ 
/* Actually just 3 components, but luminance has double sampling. */
static int saved_block[4][20][64];
static int saved_block_no = 0;

/* Parameters for converters. */
static int selected_component;
static FILE *output_file;



static void print_rgb(double y, double cb, double cr)
{
  int red, green, blue;

  /* Convert from YCbCr to RGB. */
  red   = (int)(y                          + 1.402   * (cr - 128.0));
  green = (int)(y - 0.34414 * (cb - 128.0) - 0.71414 * (cr - 128.0));
  blue  = (int)(y + 1.772   * (cb - 128.0));

  /* Cut values if they are out of range. */
  if (red > 255)
    red = 255;
  else if (red < 0)
    red = 0;

  if (green > 255)
    green = 255;
  else if (green < 0)
    green = 0;

  if (blue > 255)
    blue = 255;
  else if (blue < 0)
    blue = 0;

  fprintf(output_file, "%d %d %d ", red, green, blue);
}



void pnm_block_to_ppm(int block[], int block_no)
{
  int i, row, col, y1, y2;

  for (i = 0; i < 64; i++)
    saved_block[block_no % 4][saved_block_no][i] = block[i];

  if (block_no % 4 == 3)
    saved_block_no++;

  if (saved_block_no >= 20) { /* Entire image width collected, time to dump. */
    saved_block_no = 0;

    for (row = 0; row < 16; row++) {   /* Rows */
      for (col = 0; col < 20; col++) { /* Columns */
        for (i = 0; i < 8; i++) {      /* Values */

          /* Show the two luminance components as a chess-board combination. */
          y1 = (row % 2 == 0) ? 0 : 3;
          y2 = (row % 2 == 0) ? 3 : 0;
          
          print_rgb((double)saved_block[y1][col][((row / 2) * 8) + i],
                    (double)saved_block[1][col][((row / 2) * 8) + i],
                    (double)saved_block[2][col][((row / 2) * 8) + i]);

          print_rgb((double)saved_block[y2][col][((row / 2) * 8) + i],
                    (double)saved_block[1][col][((row / 2) * 8) + i],
                    (double)saved_block[2][col][((row / 2) * 8) + i]);
        }
        fprintf(output_file, "\n");
      }
    }
  }
}



void pnm_component_to_pgm(int block[], int block_no)
{
  int i, row, col;

  if (block_no % 4 == selected_component) {
    for (i = 0; i < 64; i++)
      saved_block[0][saved_block_no][i] = block[i];
    saved_block_no++;
  }

  if (saved_block_no >= 20) { /* Entire image width collected, time to dump. */
    saved_block_no = 0;
    for (row = 0; row < 8; row++) {    /* Rows */
      for (col = 0; col < 20; col++) { /* Columns */
        for (i = 0; i < 8; i++) {      /* Values */
          fprintf(output_file, "%d ", saved_block[0][col][(row * 8) + i]);
        }
      }
      fprintf(output_file, "\n");
    }
  }
}



/* Note: This must be run before using one of the converters! */
void pnm_init(FILE *fh, int component, char *header)
{
  output_file = fh;
  selected_component = component;
  fputs(header, output_file);
}

