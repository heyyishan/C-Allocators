#include <stdio.h>
#include <string.h>

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
