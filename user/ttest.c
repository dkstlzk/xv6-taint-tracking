#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/tlib.h"

// user/ttest.c — Process and syscall taint test harness (Member B)
//
// Plan reference: Task B-04 (compiling stub / harness; main exits 0)
// Future milestones implement process cases: T0 (hello), T1 (clean process clean),
// T3 (read > 0 taints), T4 (child inherits parent taint), T11a (write-down gate).

int
main(int argc, char *argv[])
{
  // Day 1 compilation and harness sanity check
  tlib_summary();
  exit(0);
}
