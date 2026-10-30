#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "stat.h"
#include "fs.h"
#include "file.h"
#include "defs.h"

// Fail-closed stubs for Day 1 (Task D-02)
// in_safe returns 0 so no program is considered inside /safe yet.
int
in_safe(struct inode *ip)
{
  return 0;
}

int
ip_untrusted(struct inode *ip)
{
  return ip->untrusted != 0;
}
