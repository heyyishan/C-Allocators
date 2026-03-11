#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uintmax_t *buffer;
  size_t size;
  size_t offset;
} Arena;

Arena arena_init(size_t capacity) {
  return (Arena){
      .buffer = (uintmax_t *)malloc(capacity), .size = capacity, .offset = 0};
}

void *arena_alloc(Arena *arena, size_t size) {
  // Standard 8-byte alignment
  size_t alignment = 8;
  size_t aligned_offset = (arena->offset + (alignment - 1)) & ~(alignment - 1);
  if (aligned_offset + size <= size) {
    void *ptr = &arena->buffer[aligned_offset];
    arena->offset = aligned_offset + size;
    return ptr;
  }
  return NULL;
}
void arena_reset(Arena *a) { a->offset = 0; }

void arena_destroy(Arena *a) {
  free(a->buffer);
  a->buffer = NULL;
  a->size = 0;
  a->offset = 0;
}

int main(int argc, char *argv[]) { return EXIT_SUCCESS; }
