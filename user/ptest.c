#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/tlib.h"

// user/ptest.c — Pipe and exec containment test harness (Member D)
//
// Plan reference: Task D-02 (compiling stub / harness; main exits 0)
// Future milestones implement: T6 (pipe taint), T7 (clean pipe), T8 (mixed writers),
// T10 (exec containment), T12 (taint survives exec), T14 (pipe dup), T20 (exec /safe, /).

int
main(int argc, char *argv[])
{
  tlib_summary();
  exit(0);
}
