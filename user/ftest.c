#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/tlib.h"

// user/ftest.c — Filesystem taint enforcement test suite (Member C)
//
// Plan reference: Task C-03 (stub / skeleton; main exits 0)
// Future milestones implement: T2 (two-boot persistence), T5 (create born untrusted),
// T9 (unlink refusal), T11b (open-for-write gate), T13 (inode reuse), T16 (marktaint errors),
// T18 (open truncate refusal), T19 (open RDWR refusal).

int
main(int argc, char *argv[])
{
  tlib_summary();
  exit(0);
}
