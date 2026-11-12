#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "user/tlib.h"

#define FTEST_READ_BUF     80
#define INODE_REUSE_PROBES 8
#define PERSIST_TOKEN_MAX  64

static void
require(int condition)
{
  if (!condition) {
    fprintf(2, "ftest: setup failed\n");
    exit(1);
  }
}

static void
put(char *path, char *text)
{
  int fd = open(path, O_CREATE | O_WRONLY | O_TRUNC);
  require(fd >= 0);
  require(write(fd, text, strlen(text)) == strlen(text));
  close(fd);
}

static int
equals(char *path, char *text)
{
  char data[FTEST_READ_BUF];
  int fd = open(path, O_RDONLY);
  if (fd < 0)
    return 0;
  int count = read(fd, data, sizeof(data));
  close(fd);
  return count == strlen(text) && memcmp(data, text, count) == 0;
}

static void
taint(void)
{
  put("/ftseed", "taint");
  require(marktaint("/ftseed") == 0);
  require(equals("/ftseed", "taint") && istainted() == 1);
}

static void
born_untrusted(void)
{
  unlink("/t5new");
  int child = fork(), status = -1;
  require(child >= 0);
  if (child == 0) {
    taint();
    put("/t5new", "new-data");
    exit(0);
  }
  require(wait(&status) == child && status == 0);
  require(istainted() == 0);
  CHECK("T5", "clean observer reads born-untrusted file", equals("/t5new", "new-data") && istainted() == 1);
}

static void
unlink_refused(void)
{
  put("/t9clean", "preserved");
  taint();
  CHECK("T9", "unlink refused and contents preserved", unlink("/t9clean") == -1 && equals("/t9clean", "preserved"));
}

static void
open_refused(void)
{
  put("/tclean", "preserved");
  taint();
  CHECK("T11b", "write-only open refused", open("/tclean", O_WRONLY) == -1);
  CHECK("T18", "read-only truncate refused before data loss", open("/tclean", O_RDONLY | O_TRUNC) == -1 && equals("/tclean", "preserved"));
  CHECK("T19", "read-write open refused", open("/tclean", O_RDWR) == -1);
  int fd = open("/ftseed", O_WRONLY);
  CHECK("T11b", "untrusted file remains writable", fd >= 0 && write(fd, "okay", 4) == 4);
  if (fd >= 0) close(fd);
}

static void
mark_validation(void)
{
  CHECK("T16", "missing empty directory and bad-pointer paths rejected",
        marktaint("/missing") == -1 && marktaint("") == -1 &&
        marktaint("/") == -1 && marktaint((char *)~(uint64)0) == -1);
}

static void
inode_reuse(void)
{
  struct stat original, current;
  unlink("/reuse");
  put("/reuse", "old");
  require(stat("/reuse", &original) == 0);
  require(marktaint("/reuse") == 0 && unlink("/reuse") == 0);
  int reused = 0, clean = 1;
  for (int index = 0; index < INODE_REUSE_PROBES; index++) {
    char path[] = "/reuse0";
    path[6] = '0' + index;
    unlink(path);
    put(path, "fresh");
    require(stat(path, &current) == 0);
    printf("T13 inode old=%d new=%d path=%s\n", original.ino, current.ino, path);
    reused |= original.ino == current.ino;
    clean &= equals(path, "fresh") && istainted() == 0;
  }
  if (!reused)
    INCONCLUSIVE("T13", "no inode reuse within eight allocations");
  else
    CHECK("T13", "REUSE-CONFIRMED all eight readers remain clean", clean);
}

static void
nonregular_refused(void)
{
  taint();
  CHECK("T-OH2", "tainted mkdir refused", mkdir("/bad-directory") == -1);
  CHECK("T-OH2", "tainted mknod refused", mknod("/bad-device", 1, 1) == -1);
}

static void
fp4(void)
{
  unlink("/new.txt");
  taint();
  put("/new.txt", "allowed");
  CHECK("FP-4", "tainted creator writes newly untrusted file", equals("/new.txt", "allowed"));
}

static void
phase(char *mode, char *token)
{
  require(strlen(token) > 0 && strlen(token) < PERSIST_TOKEN_MAX && istainted() == 0);
  if (strcmp(mode, "phase1-t2") == 0 || strcmp(mode, "phase1-kill") == 0) {
    put("/t2.txt", token);
    CHECK("T2-P1", "token created and marked", marktaint("/t2.txt") == 0);
    if (strcmp(mode, "phase1-kill") != 0)
      sync();
  } else if (strcmp(mode, "phase2-t2") == 0) {
    CHECK("T2", "reboot token matches and read taints fresh process", equals("/t2.txt", token) && istainted() == 1);
  } else if (strcmp(mode, "phase1-t17") == 0) {
    put("/t17proof", token);
    put("/t17a.txt", token);
    CHECK("T17-P1", "marked file deleted before reboot", marktaint("/t17a.txt") == 0 && unlink("/t17a.txt") == 0);
    sync();
  } else if (strcmp(mode, "phase2-t17") == 0) {
    struct stat state;
    int proof = equals("/t17proof", token) && istainted() == 0;
    int absent = stat("/t17a.txt", &state) == -1;
    put("/t17b.txt", "fresh");
    CHECK("T17", "same-image lifecycle and clean recreation (no reuse claim)",
          proof && absent && equals("/t17b.txt", "fresh") && istainted() == 0);
  } else {
    CHECK("ARGS", "unknown persistence phase", 0);
  }
}

int
main(int argc, char **argv)
{
  if (argc == 3)
    phase(argv[1], argv[2]);
  else if (argc == 2 && strcmp(argv[1], "fp4") == 0)
    run_in_child(fp4);
  else if (argc == 2 && strcmp(argv[1], "oh2") == 0)
    run_in_child(nonregular_refused);
  else if (argc == 1) {
    run_in_child(born_untrusted);
    run_in_child(unlink_refused);
    run_in_child(open_refused);
    run_in_child(mark_validation);
    run_in_child(inode_reuse);
    run_in_child(nonregular_refused);
  } else
    CHECK("ARGS", "invalid arguments", 0);
  tlib_summary();
  exit(0);
}
