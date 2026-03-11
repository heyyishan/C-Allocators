#include "stdio.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint8_t *data;   // The actual memory block
  size_t capacity; // Total size of the block
  size_t offset;   // Where the next allocation starts
} Arena;

uintptr_t align_forward(uintptr_t ptr, size_t align) {
  return (ptr + (align - 1)) & ~(align - 1);
}

void *arena_alloc(Arena *a, size_t size) {
  // 1. Calculate the aligned position
  uintptr_t curr_ptr = (uintptr_t)a->data + a->offset;
  uintptr_t aligned_ptr = align_forward(curr_ptr, 8); // 8-byte alignment
  // // 2. Calculate how much we moved forward due to alignment
  size_t alignment_padding = aligned_ptr - curr_ptr;
  // 3. Check if we have enough space
  if (a->offset + alignment_padding + size <= a->capacity) {
    void *ptr = (void *)aligned_ptr;
    a->offset += (alignment_padding + size);

    // Zero-initialize memory (optional but recommended)
    // memset(ptr, 0, size);
    return ptr;
  }

  return NULL; // Out of memory!
}

void arena_reset(Arena *a) { a->offset = 0; }

typedef struct {
  int id;
  char name[32];
} User;

int main() {
  // 1. Setup: Grab 1MB of memory once
  Arena my_arena = {
      .data = malloc(1024 * 1024), .capacity = 1024 * 1024, .offset = 0};

  // 2. Use: Allocate objects as if they were free!
  for (int i = 0; i < 3; i++) {
    User *u = arena_alloc(&my_arena, sizeof(User));
    if (u) {
      u->id = i;
      sprintf(u->name, "User %d", i);
      printf("Allocated: %s at address %p\n", u->name, (void *)u);
    }
  }

  // 3. Cleanup: No need to free each 'u'. Just reset the whole bucket.
  arena_reset(&my_arena);

  // Now the 1MB is ready to be reused for the next task!
  free(my_arena.data); // Only free the big block at the very end of the program
  return 0;
}
