#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "user/tlib.h"

static uint random_state = 1;

static uint
next_random(void)
{
  random_state = random_state * 1664525U + 1013904223U;
  return random_state;
}

static void
require(int condition)
{
  if (!condition) {
    fprintf(2, "ptest: setup failed\n");
    exit(1);
  }
}

static void
taint(void)
{
  int fd = open("/pseed", O_CREATE | O_WRONLY | O_TRUNC);
  require(fd >= 0 && write(fd, "T", 1) == 1);
  close(fd);
  require(marktaint("/pseed") == 0);
  fd = open("/pseed", O_RDONLY);
  char byte;
  require(fd >= 0 && read(fd, &byte, 1) == 1);
  close(fd);
  require(istainted() == 1);
}

static void
pipe_case(int untrusted, int use_dup)
{
  int fds[2], status = -1;
  require(pipe(fds) == 0);
  int child = fork();
  require(child >= 0);
  if (child == 0) {
    close(fds[0]);
    if (untrusted) taint();
    int writer = use_dup ? dup(fds[1]) : fds[1];
    require(writer >= 0 && write(writer, "T", 1) == 1);
    close(writer);
    if (use_dup) close(fds[1]);
    exit(0);
  }
  close(fds[1]);
  require(wait(&status) == child && status == 0);
  char byte;
  require(istainted() == 0);
  CHECK("PIPE-0", "zero-byte pipe read stays clean", read(fds[0], &byte, 0) == 0 && istainted() == 0);
  CHECK("PIPE-E", "failed pipe copy stays clean", read(fds[0], (void *)~(uint64)0, 1) == -1 && istainted() == 0);
  int count = read(fds[0], &byte, 1);
  close(fds[0]);
  CHECK(use_dup ? "T14" : untrusted ? "T6" : "T7", "pipe reader state matches writer and dup shares pipe", count == 1 && istainted() == untrusted);
}

static void tainted_pipe(void) { pipe_case(1, 0); }
static void clean_pipe(void) { pipe_case(0, 0); }
static void dup_pipe(void) { pipe_case(1, 1); }

static void
mixed_pipe(void)
{
  int fds[2], status = -1;
  require(pipe(fds) == 0 && write(fds[1], "CCCCC", 5) == 5);
  int child = fork();
  require(child >= 0);
  if (child == 0) {
    close(fds[0]);
    taint();
    require(write(fds[1], "TTTTT", 5) == 5);
    close(fds[1]);
    exit(0);
  }
  require(wait(&status) == child && status == 0);
  close(fds[1]);
  char bytes[5];
  CHECK("T8", "clean prefix below watermark conservatively taints", read(fds[0], bytes, 5) == 5 && memcmp(bytes, "CCCCC", 5) == 0 && istainted() == 1);
  close(fds[0]);
}

static void
exec_checks(void)
{
  mkdir("/safe");
  unlink("/safe/ptest");
  require(link("/ptest", "/safe/ptest") == 0);
  taint();
  char *outside[] = {"echo", 0};
  CHECK("T10", "exec outside safe refused", exec("/echo", outside) == -1);
  CHECK("T20", "safe directory self-match refused", exec("/safe", outside) == -1);
  CHECK("T20", "root directory refused", exec("/", outside) == -1);
  int child = fork(), status = -1;
  require(child >= 0);
  if (child == 0) {
    char *args[] = {"ptest", "exec-check", 0};
    exec("/safe/ptest", args);
    exit(2);
  }
  require(wait(&status) == child);
  CHECK("T12", "clean safe ELF executes and observes inherited taint", status == 0);
}

static void
stress_trial(void)
{
  int fds[2], statuses[2], children[2];
  require(pipe(fds) == 0 && istainted() == 0);
  uint seed = random_state;
  int first = next_random() & 1;
  for (int index = 0; index < 2; index++) {
    int untrusted = index ^ first;
    children[index] = fork();
    require(children[index] >= 0);
    if (children[index] == 0) {
      close(fds[0]);
      if (untrusted) taint();
      if ((seed >> index) & 1) pause(1);
      char bytes[1024];
      memset(bytes, untrusted ? 'T' : 'C', sizeof(bytes));
      require(write(fds[1], bytes, sizeof(bytes)) == sizeof(bytes));
      close(fds[1]);
      exit(0);
    }
  }
  close(fds[1]);
  char order[2049], bytes[127];
  int total = 0, clean = 0, marked = 0, false_negatives = 0, valid = 1;
  for (;;) {
    int count = read(fds[0], bytes, 1 + next_random() % sizeof(bytes));
    if (count == 0) break;
    if (count < 0 || total + count > 2048) { valid = 0; break; }
    for (int index = 0; index < count; index++) {
      order[total++] = bytes[index];
      if (bytes[index] == 'T') {
        marked++;
        if (!istainted()) false_negatives++;
      } else if (bytes[index] == 'C') clean++;
      else valid = 0;
    }
  }
  close(fds[0]);
  require(wait(&statuses[0]) > 0 && wait(&statuses[1]) > 0);
  order[total] = 0;
  printf("STRESS seed=%d first=%s bytes=%d clean=%d tainted=%d false_negatives=%d order=%s\n",
         seed, first ? "T" : "C", total, clean, marked, false_negatives, order);
  CHECK("STRESS", "two concurrent long writers and clean reader; zero false negatives observed",
        valid && statuses[0] == 0 && statuses[1] == 0 && clean == 1024 && marked == 1024 && false_negatives == 0 && istainted() == 1);
}

int
main(int argc, char **argv)
{
  if (argc == 2 && strcmp(argv[1], "exec-check") == 0)
    exit(istainted() == 1 ? 0 : 1);
  if (argc == 4 && strcmp(argv[1], "stress") == 0) {
    int trials = atoi(argv[2]);
    random_state = atoi(argv[3]);
    require(trials > 0 && trials <= 1000);
    for (int trial = 0; trial < trials; trial++) {
      printf("TRIAL %d seed=%d\n", trial + 1, random_state);
      run_in_child(stress_trial);
      next_random();
    }
  } else if (argc == 1) {
    run_in_child(tainted_pipe);
    run_in_child(clean_pipe);
    run_in_child(dup_pipe);
    run_in_child(mixed_pipe);
    run_in_child(exec_checks);
    run_in_child(stress_trial);
  } else
    CHECK("ARGS", "invalid arguments", 0);
  tlib_summary();
  exit(0);
}
