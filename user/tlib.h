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
// xv6 wait preserves all 32 status bits. Do not use a reporting pipe: a tainted
// child would taint the suite coordinator and contaminate later tests.
static inline int
run_in_child(void (*fn)(void))
{
  int pid = fork();
  if (pid < 0) {
    CHECK("HARNESS", "fork failed", 0);
    return -1;
  }
  if (pid == 0) {
    g_pass = g_fail = g_inconc = 0;
    fn();
    if (g_pass + g_fail + g_inconc == 0 ||
        g_pass > 255 || g_fail > 255 || g_inconc > 255)
      exit(-1);
    exit(0x01000000 | (g_inconc << 16) | (g_fail << 8) | g_pass);
  }
  int status = 0;
  if (wait(&status) != pid || (status & 0xff000000) != 0x01000000) {
    CHECK("HARNESS", "child exited without reporting checks", 0);
    return -1;
  }
  g_pass += status & 255;
  g_fail += (status >> 8) & 255;
  g_inconc += (status >> 16) & 255;
  return (status & 0x00ffff00) ? -1 : 0;
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
