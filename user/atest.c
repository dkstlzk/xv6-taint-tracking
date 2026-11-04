#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "user/user.h"
#include "user/tlib.h"

static void
require(int condition)
{
  if (!condition) { fprintf(2, "atest: setup failed\n"); exit(1); }
}

static void
put(char *path, char *text)
{
  int fd = open(path, O_CREATE | O_WRONLY | O_TRUNC);
  require(fd >= 0 && write(fd, text, strlen(text)) == strlen(text));
  close(fd);
}

static int
equals(char *path, char *text)
{
  char data[32];
  int fd = open(path, O_RDONLY);
  if (fd < 0) return 0;
  int count = read(fd, data, sizeof(data));
  close(fd);
  return count == strlen(text) && memcmp(data, text, count) == 0;
}

static void
taint(void)
{
  put("/aseed", "taint");
  require(marktaint("/aseed") == 0 && equals("/aseed", "taint") && istainted() == 1);
}

static void
wait_ok(int child)
{
  int status = -1;
  require(child >= 0 && wait(&status) == child && status == 0);
}

static void
attack_a1(void)
{
  put("/victim.txt", "preserve");
  int fds[2];
  require(pipe(fds) == 0);
  int child = fork();
  require(child >= 0);
  if (child == 0) {
    close(fds[0]); taint();
    require(write(fds[1], "T", 1) == 1);
    close(fds[1]); exit(0);
  }
  close(fds[1]); wait_ok(child);
  char byte;
  require(istainted() == 0 && read(fds[0], &byte, 1) == 1 && istainted() == 1);
  close(fds[0]);
  CHECK("A1", "pipe propagation and unlink gate preserve victim", unlink("/victim.txt") == -1 && equals("/victim.txt", "preserve"));
}

static void
attack_a2(void)
{
  put("/clean_cfg.txt", "preserve");
  unlink("/launder.txt");
  int child = fork();
  require(child >= 0);
  if (child == 0) { taint(); put("/launder.txt", "payload"); exit(0); }
  wait_ok(child);
  require(istainted() == 0);
  int fd = open("/clean_cfg.txt", O_WRONLY);
  require(fd >= 0 && equals("/launder.txt", "payload") && istainted() == 1);
  int result = write(fd, "damage", 6);
  close(fd);
  CHECK("A2", "born-untrusted inode and filewrite gate block laundering", result == -1 && equals("/clean_cfg.txt", "preserve"));
}

static void
attack_a3(void)
{
  put("/secret.txt", "secret"); put("/victim.txt", "preserve");
  unlink("/alias.txt");
  require(marktaint("/secret.txt") == 0 && link("/secret.txt", "/alias.txt") == 0);
  require(equals("/alias.txt", "secret") && istainted() == 1);
  CHECK("A3", "hard-link alias shares inode taint; unlink gate", unlink("/victim.txt") == -1 && equals("/victim.txt", "preserve"));
}

static void
attack_a4(void)
{
  mkdir("/safe"); unlink("/safe/rm"); taint();
  CHECK("A4", "link gate blocks inserting rm into safe", link("/rm", "/safe/rm") == -1);
}

static void
attack_a8(void)
{
  put("/victim.txt", "preserve"); taint();
  register uint64 argument asm("a0") = (uint64)"/victim.txt";
  register uint64 number asm("a7") = SYS_unlink;
  asm volatile("ecall" : "+r"(argument) : "r"(number) : "memory");
  CHECK("A8", "raw ecall reaches kernel unlink gate", (int)argument == -1 && equals("/victim.txt", "preserve"));
}

static void
attack_a9(void)
{
  mkdir("/safe"); unlink("/safe/atest");
  require(link("/atest", "/safe/atest") == 0);
  put("/victim.txt", "preserve"); taint();
  int child = fork();
  require(child >= 0);
  if (child == 0) {
    char *args[] = {"atest", "survive", 0};
    exec("/safe/atest", args);
    exit(2);
  }
  int status = -1;
  require(wait(&status) == child);
  CHECK("A9", "taint survives allowed exec; unlink and open gates hold", status == 0 && equals("/victim.txt", "preserve"));
}

static void
attack_a16(void)
{
  mkdir("/safe"); unlink("/safe/evil");
  int child = fork();
  require(child >= 0);
  if (child == 0) {
    taint();
    int source = open("/echo", O_RDONLY);
    int target = open("/safe/evil", O_CREATE | O_WRONLY);
    require(source >= 0 && target >= 0);
    char bytes[512]; int count, total = 0;
    while ((count = read(source, bytes, sizeof(bytes))) > 0) {
      require(write(target, bytes, count) == count); total += count;
    }
    require(count == 0 && total > 512);
    close(source); close(target); exit(0);
  }
  wait_ok(child);
  child = fork();
  require(child >= 0);
  if (child == 0) {
    char *args[] = {"evil", "valid-ELF-control", 0};
    exec("/safe/evil", args); exit(2);
  }
  wait_ok(child);
  taint();
  char *args[] = {"evil", 0};
  CHECK("A16", "born-untrusted valid ELF refused inside safe", exec("/safe/evil", args) == -1);
}

static void
attack_a17(void)
{
  put("/clean.txt", "preserve"); taint();
  CHECK("A17", "create never relabels existing clean inode", open("/clean.txt", O_CREATE | O_WRONLY) == -1 && equals("/clean.txt", "preserve"));
}

static void
attack_a18(void)
{
  put("/clean.txt", "preserve"); taint();
  CHECK("A18", "open gate precedes truncation", open("/clean.txt", O_WRONLY | O_TRUNC) == -1 && equals("/clean.txt", "preserve"));
}

int
main(int argc, char **argv)
{
  if (argc == 2 && strcmp(argv[1], "survive") == 0)
    exit(istainted() == 1 && unlink("/victim.txt") == -1 && open("/victim.txt", O_WRONLY) == -1 ? 0 : 1);
  if (argc == 2 && strcmp(argv[1], "a8") == 0)
    run_in_child(attack_a8);
  else if (argc == 1) {
    run_in_child(attack_a1); run_in_child(attack_a2); run_in_child(attack_a3);
    run_in_child(attack_a4); run_in_child(attack_a8); run_in_child(attack_a9);
    run_in_child(attack_a16); run_in_child(attack_a17); run_in_child(attack_a18);
  } else
    CHECK("ARGS", "invalid arguments", 0);
  tlib_summary(); exit(0);
}
