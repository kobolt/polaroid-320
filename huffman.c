#include "huffman.h"
#include <stdlib.h>
#include <error.h>
#include <stdio.h>
#include <string.h>



/* Returns 1 when a node was allocated, 0 if not allocated, or -1 on error. */
static int huffman_allocate(huffman_t *node, int depth, int value)
{
  /* First time the node is seen, allocate it. */
  if (node->zero == NULL) {
    node->zero = malloc(sizeof(huffman_t));
    if (node->zero == NULL)
      error(1, 0, "%s.%d: malloc() failed.", __FILE__, __LINE__);
    node->zero->zero  = NULL;
    node->zero->one   = NULL;
    node->zero->value = -1;
  }

  if (node->one == NULL) {
    node->one = malloc(sizeof(huffman_t));
    if (node->one == NULL)
      error(1, 0, "%s.%d: malloc() failed.", __FILE__, __LINE__);
    node->one->zero  = NULL;
    node->one->one   = NULL;
    node->one->value = -1;
  }

  if (depth > 0) {
    if (node->value != -1)
      return 0; /* This one is used, move along. */

    if (huffman_allocate(node->zero, depth - 1, value) == 1)
      return 1;
    if (huffman_allocate(node->one,  depth - 1, value) == 1)
      return 1;
  } else { /* depth == 0 */

    if (node->value == -1) {
      node->value = value;
      return 1;
    } else
      return 0; /* This one is used, move along. */
  }

  return -1; /* No space left in tree to allocate the value. */
}



/* Function to convert from the table format to the binary tree format. */
huffman_t *huffman_convert_table(unsigned char huffman_table[])
{
  int i, j, n;
  huffman_t *root;

  root = malloc(sizeof(huffman_t));
  if (root == NULL)
    error(1, 0, "%s.%d: malloc() failed.", __FILE__, __LINE__);

  /* The root node cannot really have a value, but set it to unused anyway. */
  root->value = -1;
  root->zero  = NULL;
  root->one   = NULL;

  n = 0;
  for (i = 0; i < 16; i++) {
    for (j = 0; j < huffman_table[i]; j++) {
      if (huffman_allocate(root, i + 1, huffman_table[16 + n]) != 1)
        error(1, 0, "%s.%d: Unable to allocate huffman node",
          __FILE__, __LINE__);
      n++;
    }
  }

  return root;
}



/* Returns address of 1 (one) or 0 (zero) node if the complete value is not
  found. When the value is found, NULL is returned and the value is placed in
  the "value" variable pointed to. */
huffman_t *huffman_lookup(huffman_t *node, int bit, int *value)
{
  if (node == NULL) {
    error(0, 0, "%s.%d: NULL node in lookup.", __FILE__, __LINE__);
    *value = -1;
    return NULL;
  }

  if (bit == 0) {
    if (node->zero == NULL) {
      error(0, 0, "%s.%d: NULL node in lookup.", __FILE__, __LINE__);
      *value = -1;
      return NULL;
    }

    if (node->zero->value == -1)
      return node->zero; /* More bits needed. */

    *value = node->zero->value;
    return NULL;

  } else if (bit == 1) {
    if (node->one == NULL) {
      error(0, 0, "%s.%d: NULL node in lookup.", __FILE__, __LINE__);
      *value = -1;
      return NULL;
    }

    if (node->one->value == -1)
      return node->one; /* More bits needed. */

    *value = node->one->value;
    return NULL;

  }

  error(0, 0, "%s.%d: Invalid bit value.", __FILE__, __LINE__);
  *value = -1;
  return NULL;
}



static void huffman_node_free(huffman_t *node)
{
  if (node->zero != NULL) {
    huffman_node_free(node->zero);
    free(node->zero);
  }
  if (node->one != NULL) {
    huffman_node_free(node->one);
    free(node->one);
  }
}



void huffman_tree_free(huffman_t *root)
{
  if (root == NULL)
    return;
  huffman_node_free(root);
  free(root);
}

