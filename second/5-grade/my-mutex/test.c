#include <stdatomic.h>
#include <stdio.h>

int main() {
  atomic_int x = 10;
  printf("%d\n", __sync_val_compare_and_swap(&x, 10, 40));
  printf("%d\n", x);
  printf("%d\n", atomic_exchange(&x, 1000));
  printf("%d\n", x);
  printf("%d\n", atomic_fetch_sub(&x, 10));
  printf("%d\n", x);
  return 0;
}
