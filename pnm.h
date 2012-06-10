#ifndef _PNM_H
#define _PNM_H

#include <stdio.h>

void pnm_block_to_ppm(int block[], int block_no);
void pnm_component_to_pgm(int block[], int block_no);
void pnm_init(FILE *fh, int component, char *header);

#endif /* _PNM_H */
