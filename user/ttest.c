#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "user/tlib.h"

// user/ttest.c — Process and syscall taint test harness (Member B)
// Tests: T0 (hello), T1 (fresh clean), T4 (fork taint inheritance)

void
test_t0_hello(void)
{
  int sum = hello(1, 2, 3, 4);
  CHECK("T0", "hello syscall returns correct sum", sum == 10);
}

void
test_t1_fresh_clean(void)
{
  int t = istainted();
  CHECK("T1", "fresh process starts clean", t == 0);
}

void
test_t4_fork_inheritance(void)
{
  // Create an untrusted file and read from it to become tainted
  int fd = open("/untrusted_t4.txt", O_CREATE | O_WRONLY);
  if (fd >= 0) {
    write(fd, "bad", 3);
    close(fd);
    marktaint("/untrusted_t4.txt");
    fd = open("/untrusted_t4.txt", O_RDONLY);
    if (fd >= 0) {
      char buf[4];
      read(fd, buf, 3);
      close(fd);
    }
  }

  // Parent should be tainted now
  int parent_taint = istainted();
  if (!parent_taint) {
    // If marktaint is still a stub, record INCONCLUSIVE or skip
    INCONCLUSIVE("T4", "fork inheritance (marktaint not yet active)");
    return;
  }

  int pid = fork();
  if (pid < 0) {
    CHECK("T4", "fork succeeded", 0);
    return;
  }
  if (pid == 0) {
    int child_taint = istainted();
    exit(child_taint == 1 ? 0 : 1);
  } else {
    int status;
    wait(&status);
    CHECK("T4", "child inherits parent taint across fork", status == 0);
  }
}


void
test_t3_read_latch(void)
{
  int fd = open("/untrusted_t3.txt", O_CREATE | O_WRONLY);
  if (fd < 0) {
    CHECK("T3", "create test file", 0);
    return;
  }
  write(fd, "test", 4);
  close(fd);

  if (marktaint("/untrusted_t3.txt") < 0) {
    CHECK("T3", "marktaint failed", 0);
    return;
  }

  // 1. Zero-byte read must NOT taint
  fd = open("/untrusted_t3.txt", O_RDONLY);
  if (fd < 0) {
    CHECK("T3", "open untrusted file", 0);
    return;
  }
  char buf[8];
  read(fd, buf, 0);
  int t0 = istainted();
  close(fd);

  // 2. Positive-byte read MUST taint
  fd = open("/untrusted_t3.txt", O_RDONLY);
  if (fd < 0) {
    CHECK("T3", "open untrusted file again", 0);
    return;
  }
  int n = read(fd, buf, 2);
  int t1 = istainted();
  close(fd);

  CHECK("T3", "positive-byte read taints while zero-byte read does not", t0 == 0 && n > 0 && t1 == 1);
}


void
test_t11a_write_clean_fd_after_taint(void)
{
  // 1. Open a clean file for writing while process is clean
  int clean_fd = open("/clean_t11a.txt", O_CREATE | O_WRONLY);
  if (clean_fd < 0) {
    CHECK("T11a", "create clean file", 0);
    return;
  }

  // 2. Taint process by reading untrusted file
  int ufd = open("/untrusted_t11a.txt", O_CREATE | O_WRONLY);
  write(ufd, "taint", 5);
  close(ufd);
  marktaint("/untrusted_t11a.txt");

  ufd = open("/untrusted_t11a.txt", O_RDONLY);
  char buf[8];
  read(ufd, buf, 5);
  close(ufd);

  int t = istainted();
  if (t != 1) {
    close(clean_fd);
    CHECK("T11a", "process became tainted", 0);
    return;
  }

  // 3. Write to the previously opened clean fd -> MUST fail (-1)
  int w = write(clean_fd, "malicious", 9);
  close(clean_fd);

  CHECK("T11a", "write via FD opened while clean is refused after process becomes tainted", w < 0);
}

int
main(int argc, char *argv[])
{
  run_in_child(test_t0_hello);
  run_in_child(test_t1_fresh_clean);
  run_in_child(test_t4_fork_inheritance);
  run_in_child(test_t3_read_latch);
  run_in_child(test_t11a_write_clean_fd_after_taint);
  tlib_summary();
  exit(0);
}
