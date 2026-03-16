#include "v_pool.h"
#include <assert.h>
#include <stdlib.h>

typedef struct {
  struct v_node *next;
} v_node;

v_pool *v_pool_create(size_t chunk_size, size_t count) {
  size_t actual_size =
      (chunk_size < sizeof(v_node)) ? sizeof(v_node) : chunk_size;

  v_pool *p = malloc(sizeof(v_pool));
  if (p == nullptr)
    return nullptr;

  p->buffer = malloc(actual_size * count);
  if (p->buffer == nullptr) {
    free(p);
    return nullptr;
  }

  p->chunk_size = actual_size;
  p->total_chunks = count;

  p->free_list = (v_node *)p->buffer;
  v_node *curr = p->free_list;

  for (size_t i = 0; i < count - 1; i++) {
    curr->next = (v_node *)((uint8_t *)curr + actual_size);
    curr = curr->next;
  }
  curr->next = nullptr; // End of the line

  return p;
}

void *v_pool_alloc(v_pool *p) {
  if (p->free_list == nullptr)
    return nullptr; // Out of cubes in the tray

  v_node *node = p->free_list;
  p->free_list = node->next;

  return (void *)node;
}

void v_pool_free(v_pool *p, void *ptr) {
  if (ptr == nullptr)
    return;
  v_node *node = (v_node *)ptr;
  node->next = p->free_list;
  p->free_list = node;
}

void v_pool_destroy(v_pool *p) {
  if (p != nullptr) {
    free(p->buffer);
    free(p);
  }
}
