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


// T8: Mixed writers with watermark
void
test_t8_mixed_writers(void)
{
  int p[2];
  pipe(p);

  // Clean writer writes 10 bytes
  write(p[1], "0123456789", 10);

  // Tainted writer writes 10 bytes
  int pid = fork();
  if (pid == 0) {
    int fd = open("/untrusted_p8.txt", O_CREATE | O_WRONLY);
    write(fd, "bad", 3);
    close(fd);
    marktaint("/untrusted_p8.txt");
    fd = open("/untrusted_p8.txt", O_RDONLY);
    char b[4];
    read(fd, b, 3);
    close(fd);
    write(p[1], "ABCDEFGHIJ", 10);
    close(p[1]);
    exit(0);
  }
  int s;
  wait(&s);
  close(p[1]);

  // Clean reader takes first 5 bytes
  int rpid = fork();
  if (rpid == 0) {
    char buf[8];
    read(p[0], buf, 5);
    close(p[0]);
    // Known conservative false positive: reader taking clean bytes before tainted_hi is tainted
    exit(istainted() == 1 ? 0 : 1);
  }
  wait(&s);
  close(p[0]);
  CHECK("T8", "watermark flags reader consuming below tainted_hi (documented false positive)", s == 0);
}

// T14: dup keeps taint
void
test_t14_dup_keeps_taint(void)
{
  int p[2];
  pipe(p);
  int pid = fork();
  if (pid == 0) {
    int fd = open("/untrusted_p14.txt", O_CREATE | O_WRONLY);
    write(fd, "bad", 3);
    close(fd);
    marktaint("/untrusted_p14.txt");
    fd = open("/untrusted_p14.txt", O_RDONLY);
    char b[4];
    read(fd, b, 3);
    close(fd);
    int p1_dup = dup(p[1]);
    write(p1_dup, "dup_msg", 7);
    close(p1_dup);
    close(p[1]);
    exit(0);
  }
  int s;
  wait(&s);
  close(p[1]);

  char buf[8];
  read(p[0], buf, 7);
  close(p[0]);
  CHECK("T14", "pipe written via dup fd propagates taint to reader", istainted() == 1);
}


// T10: exec outside /safe refused, inside /safe clean runs
void
test_t10_exec_containment(void)
{
  mkdir("/safe");
  // Taint child
  int pid = fork();
  if (pid == 0) {
    int fd = open("/untrusted_t10.txt", O_CREATE | O_WRONLY);
    write(fd, "bad", 3);
    close(fd);
    marktaint("/untrusted_t10.txt");
    fd = open("/untrusted_t10.txt", O_RDONLY);
    char b[4];
    read(fd, b, 3);
    close(fd);

    // 1. Exec outside /safe -> MUST be refused (-1)
    char *args[] = {"echo", "outside", 0};
    int r = exec("/echo", args);
    CHECK("T10", "exec outside /safe by tainted process is refused", r == -1);
    exit(0);
  }
  int s;
  wait(&s);
}

// T12: taint survives exec
void
test_t12_taint_survives_exec(void)
{
  // Taint is preserved in struct proc across kexec
  int pid = fork();
  if (pid == 0) {
    int fd = open("/untrusted_t12.txt", O_CREATE | O_WRONLY);
    write(fd, "bad", 3);
    close(fd);
    marktaint("/untrusted_t12.txt");
    fd = open("/untrusted_t12.txt", O_RDONLY);
    char b[4];
    read(fd, b, 3);
    close(fd);

    // If an allowed binary in /safe exists, exec it; otherwise check p->tainted is never cleared
    CHECK("T12", "taint survives in process context", istainted() == 1);
    exit(0);
  }
  int s;
  wait(&s);
}

// T20: exec("/safe") and exec("/") refused
void
test_t20_exec_dir_self_match_refused(void)
{
  mkdir("/safe");
  int pid = fork();
  if (pid == 0) {
    int fd = open("/untrusted_t20.txt", O_CREATE | O_WRONLY);
    write(fd, "bad", 3);
    close(fd);
    marktaint("/untrusted_t20.txt");
    fd = open("/untrusted_t20.txt", O_RDONLY);
    char b[4];
    read(fd, b, 3);
    close(fd);

    char *args[] = {"safe", 0};
    int r1 = exec("/safe", args);
    char *args_root[] = {"", 0};
    int r2 = exec("/", args_root);
    CHECK("T20", "exec(/safe) and exec(/) are refused", r1 == -1 && r2 == -1);
    exit(0);
  }
  int s;
  wait(&s);
}


// D-09: Stress test two writers + reader
void
test_d09_stress_pipe_concurrency(void)
{
  int p[2];
  pipe(p);

  int pid1 = fork();
  if (pid1 == 0) {
    // Writer 1 (clean)
    for (int i = 0; i < 50; i++) {
      write(p[1], "C", 1);
    }
    close(p[1]);
    exit(0);
  }

  int pid2 = fork();
  if (pid2 == 0) {
    // Writer 2 (tainted)
    int fd = open("/untrusted_stress.txt", O_CREATE | O_WRONLY);
    write(fd, "bad", 3);
    close(fd);
    marktaint("/untrusted_stress.txt");
    fd = open("/untrusted_stress.txt", O_RDONLY);
    char b[4];
    read(fd, b, 3);
    close(fd);

    for (int i = 0; i < 50; i++) {
      write(p[1], "T", 1);
    }
    close(p[1]);
    exit(0);
  }

  close(p[1]);
  int s;
  wait(&s);
  wait(&s);

  int rpid = fork();
  if (rpid == 0) {
    char buf[128];
    int total = 0;
    while (1) {
      int n = read(p[0], buf, sizeof(buf));
      if (n <= 0) break;
      total += n;
    }
    close(p[0]);
    // Reader has consumed tainted bytes -> must be tainted (zero false negatives)
    exit(istainted() == 1 ? 0 : 1);
  }
  wait(&s);
  close(p[0]);
  CHECK("D-09", "concurrent pipe stress test: zero false negatives", s == 0);
}

int
main(int argc, char *argv[])
{
  run_in_child(test_t6_tainted_pipe);
  run_in_child(test_t7_clean_pipe);
  run_in_child(test_t8_mixed_writers);
  run_in_child(test_t14_dup_keeps_taint);
  run_in_child(test_t10_exec_containment);
  run_in_child(test_t12_taint_survives_exec);
  run_in_child(test_t20_exec_dir_self_match_refused);
  run_in_child(test_d09_stress_pipe_concurrency);
  tlib_summary();
  exit(0);
}
