#include "stddef.h"
#include "stdint.h"
#include "stdio.h"

typedef struct {
  struct ArenaBlock *next;
  size_t capacity;
  size_t used;
  uint8_t data[];
} ArenaBlock;

typedef struct {
  ArenaBlock *current;
  ArenaBlock *first;
  size_t default_block_size;
  size_t total_allocated;
  size_t block_count;
} Arena;

Arena arena_create(size_t default_block_size) {
  return (Arena){
      .current = NULL,
      .first = NULL,
      .default_block_size = default_block_size ? default_block_size : 64 * 1024,
      .total_allocated = 0,
      .block_count = 0,
  };
}

static ArenaBlock *arena_add_block(Arena *a, size_t min_size) {
  size_t size = a->default_block_size;
  if (min_size > size)
    size = min_size;

  ArenaBlock *block = malloc(sizeof(ArenaBlock) + size);
  if (!block)
    return NULL;

  block->next = NULL;
  block->capacity = size;
  block->used = 0;

  if (a->current) {
    a->current->next = block;
  } else {
    a->first = block;
  }
  a->current = block;
  a->block_count++;
  return block;
}

void *arena_alloc(Arena *a, size_t size, size_t align) {
  if (size == 0)
    return NULL;

  ArenaBlock *block = a->current;

  // Try current block
  if (block) {
    uintptr_t base = (uintptr_t)(block->data + block->used);
    uintptr_t aligned = (base + align - 1) & ~(align - 1);
    size_t padding = aligned - base;

    if (block->used + padding + size <= block->capacity) {
      block->used += padding + size;
      a->total_allocated += size;
      return (void *)aligned;
    }
  }

  // Need a new block
  block = arena_add_block(a, size + align);
  if (!block)
    return NULL;

  uintptr_t base = (uintptr_t)block->data;
  uintptr_t aligned = (base + align - 1) & ~(align - 1);
  size_t padding = aligned - base;

  block->used = padding + size;
  a->total_allocated += size;
  return (void *)aligned;
}

// Typed allocation helpers
#define arena_new(arena, T) ((T *)arena_alloc((arena), sizeof(T), _Alignof(T)))

#define arena_array(arena, T, count)                                           \
  ((T *)arena_alloc((arena), sizeof(T) * (count), _Alignof(T)))

// Duplicate a string into the arena
char *arena_strdup(Arena *a, const char *s) {
  size_t len = strlen(s) + 1;
  char *copy = arena_alloc(a, len, 1);
  if (copy)
    memcpy(copy, s, len);
  return copy;
}

char *arena_sprintf(Arena *a, const char *fmt, ...) {
  va_list args1, args2;
  va_start(args1, fmt);
  va_copy(args2, args1);

  int len = vsnprintf(NULL, 0, fmt, args1);
  va_end(args1);

  if (len < 0) {
    va_end(args2);
    return NULL;
  }

  char *buf = arena_alloc(a, (size_t)len + 1, 1);
  if (buf) {
    vsnprintf(buf, (size_t)len + 1, fmt, args2);
  }
  va_end(args2);
  return buf;
}

// Temporary arena: save a checkpoint, roll back later
typedef struct {
  ArenaBlock *block;
  size_t used;
  size_t total_allocated;
} ArenaCheckpoint;

ArenaCheckpoint arena_save(const Arena *a) {
  return (ArenaCheckpoint){
      .block = a->current,
      .used = a->current ? a->current->used : 0,
      .total_allocated = a->total_allocated,
  };
}

void arena_restore(Arena *a, ArenaCheckpoint cp) {
  // Free blocks added after checkpoint
  if (cp.block) {
    ArenaBlock *block = cp.block->next;
    while (block) {
      ArenaBlock *next = block->next;
      free(block);
      a->block_count--;
      block = next;
    }
    cp.block->next = NULL;
    cp.block->used = cp.used;
    a->current = cp.block;
  } else {
    // Checkpoint was before any blocks
    ArenaBlock *block = a->first;
    while (block) {
      ArenaBlock *next = block->next;
      free(block);
      block = next;
    }
    a->first = NULL;
    a->current = NULL;
    a->block_count = 0;
  }
  a->total_allocated = cp.total_allocated;
}

void arena_destroy(Arena *a) {
  ArenaBlock *block = a->first;
  while (block) {
    ArenaBlock *next = block->next;
    free(block);
    block = next;
  }
  *a = (Arena){0};
}

void pattern_21_arena(void) {
  Arena arena = arena_create(4096);

  // Fast allocation — no individual frees needed
  int *numbers = arena_array(&arena, int, 100);
  for (int i = 0; i < 100; i++)
    numbers[i] = i * i;

  char *name = arena_strdup(&arena, "Hello, World!");
  char *msg = arena_sprintf(&arena, "User %s has %d points", name, 42);

  printf("Allocated: %zu bytes in %zu blocks\n", arena.total_allocated,
         arena.block_count);

  // Checkpoint for temporary work
  ArenaCheckpoint cp = arena_save(&arena);

  char *temp = arena_array(&arena, char, 10000);
  printf("After temp: %zu bytes\n", arena.total_allocated);

  arena_restore(&arena, cp);
  printf("After restore: %zu bytes\n", arena.total_allocated);

  arena_destroy(&arena); // free everything at once
}
typedef struct PoolFreeNode {
  struct PoolFreeNode *next;
} PoolFreeNode;

typedef struct {
  uint8_t *buffer;
  size_t block_size; // size of each block (>= sizeof(PoolFreeNode))
  size_t block_count;
  PoolFreeNode *free_head;
  size_t used_count;
} Pool;

Pool pool_create(size_t block_size, size_t block_count) {
  // Blocks must be large enough to hold the free-list pointer
  if (block_size < sizeof(PoolFreeNode))
    block_size = sizeof(PoolFreeNode);

  // Align block_size to pointer alignment
  block_size =
      (block_size + _Alignof(max_align_t) - 1) & ~(_Alignof(max_align_t) - 1);

  uint8_t *buffer = malloc(block_size * block_count);
  assert(buffer);

  // Build free list by threading through all blocks
  PoolFreeNode *head = NULL;
  for (size_t i = block_count; i > 0; i--) {
    PoolFreeNode *node = (PoolFreeNode *)(buffer + (i - 1) * block_size);
    node->next = head;
    head = node;
  }

  return (Pool){
      .buffer = buffer,
      .block_size = block_size,
      .block_count = block_count,
      .free_head = head,
      .used_count = 0,
  };
}

void *pool_alloc(Pool *pool) {
  if (!pool->free_head)
    return NULL; // full

  PoolFreeNode *node = pool->free_head;
  pool->free_head = node->next;
  pool->used_count++;

  memset(node, 0, pool->block_size); // clear before returning
  return node;
}

void pool_free(Pool *pool, void *ptr) {
  if (!ptr)
    return;

  // Debug check: verify ptr is within our buffer
  assert((uint8_t *)ptr >= pool->buffer);
  assert((uint8_t *)ptr < pool->buffer + pool->block_size * pool->block_count);
  assert(((uint8_t *)ptr - pool->buffer) % pool->block_size == 0);

  PoolFreeNode *node = ptr;
  node->next = pool->free_head;
  pool->free_head = node;
  pool->used_count--;
}

void pool_destroy(Pool *pool) {
  free(pool->buffer);
  *pool = (Pool){0};
}

void pattern_22_pool(void) {
  typedef struct {
    float x, y, z;
    int id;
  } Particle;

  Pool particle_pool = pool_create(sizeof(Particle), 10000);

  // Allocate particles
  Particle *particles[100];
  for (int i = 0; i < 100; i++) {
    particles[i] = pool_alloc(&particle_pool);
    particles[i]->x = (float)i;
    particles[i]->y = (float)i * 2;
    particles[i]->id = i;
  }

  printf("Used: %zu / %zu blocks\n", particle_pool.used_count,
         particle_pool.block_count);

  // Free every other one
  for (int i = 0; i < 100; i += 2) {
    pool_free(&particle_pool, particles[i]);
    particles[i] = NULL;
  }

  printf("After freeing: %zu / %zu blocks\n", particle_pool.used_count,
         particle_pool.block_count);

  // Reallocate — will reuse freed blocks
  for (int i = 0; i < 100; i += 2) {
    particles[i] = pool_alloc(&particle_pool);
    particles[i]->id = i + 1000;
  }

  pool_destroy(&particle_pool);
}

int main(int argc, char *argv[]) { return EXIT_SUCCESS; }
