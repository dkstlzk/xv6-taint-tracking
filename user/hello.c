#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int sum = hello(1, 2, 3, 4);
  printf("sum = %d\n", sum);
  exit(0);
}
