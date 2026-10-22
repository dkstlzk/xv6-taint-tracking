#ifndef _TLIB_H_
#define _TLIB_H_

// user/tlib.h — Shared test harness skeleton for xv6 taint-tracking tests
//
// Owned by: Member B
// Plan reference: Task B-04, Appendix app:tlib, Invariant N2
//
// Uniform output convention:
//   PASS <test_id> <test_name>
//   FAIL <test_id> <test_name>
//   INCONCLUSIVE <test_id> <test_name>
//   SUMMARY pass=N fail=M inconclusive=K

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static int g_pass = 0;
static int g_fail = 0;
static int g_inconc = 0;

#define CHECK(id, name, cond) do { \
  if (cond) { \
    printf("PASS %s %s\n", (id), (name)); \
    g_pass++; \
  } else { \
    printf("FAIL %s %s\n", (id), (name)); \
    g_fail++; \
  } \
} while (0)

#define INCONCLUSIVE(id, name) do { \
  printf("INCONCLUSIVE %s %s\n", (id), (name)); \
  g_inconc++; \
} while (0)

// Run fn() in a forked child so its taint or crash never leaks into the runner.
// Child exit status encodes the check result; parent waits and returns child status.
static inline int
run_in_child(void (*fn)(void))
{
  int pid = fork();
  if (pid < 0) {
    printf("tlib: run_in_child fork failed\n");
    return -1;
  }
  if (pid == 0) {
    fn();
    exit(0);
  }
  int status = 0;
  wait(&status);
  return status;
}

// Print summary line and exit with non-zero if any test failed or was inconclusive.
static inline void
tlib_summary(void)
{
  printf("SUMMARY pass=%d fail=%d inconclusive=%d\n", g_pass, g_fail, g_inconc);
  if (g_fail > 0 || g_inconc > 0) {
    exit(1);
  }
}

#endif // _TLIB_H_
