#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(2, "Usage: mark <path>\n");
    exit(1);
  }
  if (marktaint(argv[1]) < 0) {
    fprintf(2, "mark: failed to mark %s\n", argv[1]);
    exit(1);
  }
  exit(0);
}
