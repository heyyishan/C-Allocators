#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct Point {
  int x, y;
} Point;

typedef int (*Operation)(int a, int b);

int eval(int a, int b, Operation op) { return op(a, b); }
int add(int a, int b) { return a + b; }
int main(void) {
  Point p1 = {.x = 2, .y = 5};
  Point p2 = {.x = 0, .y = 7};
  eval(2, 5, add);
  printf("%d\n", p1.x);
}
