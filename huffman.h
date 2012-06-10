#ifndef _HUFFMAN_H
#define _HUFFMAN_H

typedef struct huffman_s {
  struct huffman_s *zero; /* Left  = 0 */
  struct huffman_s *one;  /* Right = 1 */
  int value;              /* Should be -1 when not in use. */
} huffman_t;

huffman_t *huffman_convert_table(unsigned char huffman_table[]);
huffman_t *huffman_lookup(huffman_t *node, int bit, int *value);
void huffman_tree_free(huffman_t *root);

#endif /* _HUFFMAN_H */
