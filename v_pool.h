#ifndef V_POOL_H
#define V_POOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  char *buffer;
  struct v_node *free_list;
  size_t chunk_size;
  size_t total_chunks;
} v_pool;

[[nodiscard]] v_pool *v_pool_create(size_t chunk_size, size_t count);
[[nodiscard]] void *v_pool_alloc(v_pool *p);
void v_pool_free(v_pool *p, void *ptr);
void v_pool_destroy(v_pool *p);

#endif
