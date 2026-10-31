#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "user/tlib.h"

// user/ptest.c — Pipe and exec containment test harness (Member D)
// Tests: T6 (tainted pipe writer), T7 (clean pipe writer)

void
test_t6_tainted_pipe(void)
{
  int p[2];
  if (pipe(p) < 0) {
    CHECK("T6", "pipe creation", 0);
    return;
  }

  int pid = fork();
  if (pid < 0) {
    CHECK("T6", "fork", 0);
    return;
  }
  if (pid == 0) {
    // Taint child by creating and reading untrusted file
    int fd = open("/untrusted_p6.txt", O_CREATE | O_WRONLY);
    write(fd, "bad", 3);
    close(fd);
    marktaint("/untrusted_p6.txt");
    fd = open("/untrusted_p6.txt", O_RDONLY);
    char buf[4];
    read(fd, buf, 3);
    close(fd);

    close(p[0]);
    write(p[1], "msg", 3);
    close(p[1]);
    exit(0);
  } else {
    close(p[1]);
    int status;
    wait(&status);

    char buf[4];
    int n = read(p[0], buf, 3);
    close(p[0]);

    int reader_taint = istainted();
    CHECK("T6", "clean reader of pipe written by tainted writer becomes tainted", n > 0 && reader_taint == 1);
  }
}

void
test_t7_clean_pipe(void)
{
  int p[2];
  if (pipe(p) < 0) {
    CHECK("T7", "pipe creation", 0);
    return;
  }

  int pid = fork();
  if (pid < 0) {
    CHECK("T7", "fork", 0);
    return;
  }
  if (pid == 0) {
    // Clean child writes to pipe
    close(p[0]);
    write(p[1], "clean", 5);
    close(p[1]);
    exit(0);
  } else {
    close(p[1]);
    int status;
    wait(&status);

    char buf[6];
    int n = read(p[0], buf, 5);
    close(p[0]);

    int reader_taint = istainted();
    CHECK("T7", "clean reader of clean-only pipe remains clean", n > 0 && reader_taint == 0);
  }
}

int
main(int argc, char *argv[])
{
  run_in_child(test_t6_tainted_pipe);
  run_in_child(test_t7_clean_pipe);
  tlib_summary();
  exit(0);
}
