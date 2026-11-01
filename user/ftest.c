#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "user/tlib.h"

// user/ftest.c — Filesystem taint enforcement test suite (Member C)
// Tests: T5, T9, T11b, T16, T18, T19, and T13 (inode reuse)

static void
taint_myself(void)
{
  int fd = open("/untrusted_ftest_seed.txt", O_CREATE | O_WRONLY);
  write(fd, "bad", 3);
  close(fd);
  marktaint("/untrusted_ftest_seed.txt");
  fd = open("/untrusted_ftest_seed.txt", O_RDONLY);
  char buf[4];
  read(fd, buf, 3);
  close(fd);
}

// T5: create by tainted caller is born untrusted
void
test_t5_create_born_untrusted(void)
{
  taint_myself();
  int fd = open("/t5_created.txt", O_CREATE | O_WRONLY);
  if (fd < 0) {
    CHECK("T5", "create file by tainted caller", 0);
    return;
  }
  write(fd, "data", 4);
  close(fd);

  // Clean child reads it and checks if it becomes tainted
  int pid = fork();
  if (pid == 0) {
    int rfd = open("/t5_created.txt", O_RDONLY);
    char buf[8];
    read(rfd, buf, 4);
    close(rfd);
    exit(istainted() == 1 ? 0 : 1);
  } else {
    int status;
    wait(&status);
    CHECK("T5", "file created by tainted caller is born untrusted on disk", status == 0);
  }
}

// T9: tainted caller cannot unlink
void
test_t9_unlink_refused(void)
{
  int fd = open("/t9_clean.txt", O_CREATE | O_WRONLY);
  write(fd, "clean", 5);
  close(fd);

  taint_myself();
  int r = unlink("/t9_clean.txt");
  CHECK("T9", "tainted process unlink is refused with -1", r == -1);

  // Verify file survives
  fd = open("/t9_clean.txt", O_RDONLY);
  CHECK("T9", "file survives denied unlink", fd >= 0);
  if (fd >= 0) close(fd);
}

// T11b: tainted open-for-write on clean file refused
void
test_t11b_open_write_refused(void)
{
  int fd = open("/t11b_clean.txt", O_CREATE | O_WRONLY);
  write(fd, "initial", 7);
  close(fd);

  taint_myself();
  int bad_fd = open("/t11b_clean.txt", O_WRONLY);
  CHECK("T11b", "tainted open for WRONLY on clean file is refused", bad_fd < 0);
}

// T16: marktaint validation error cases
void
test_t16_marktaint_validation(void)
{
  int r1 = marktaint("/nonexistent_file_xyz.txt");
  int r2 = marktaint("");
  int r3 = marktaint("/"); // directory
  CHECK("T16", "marktaint returns -1 on missing, empty, or directory path", r1 < 0 && r2 < 0 && r3 < 0);
}

// T18: tainted open with O_TRUNC refused and contents unchanged
void
test_t18_open_trunc_refused(void)
{
  int fd = open("/t18_clean.txt", O_CREATE | O_WRONLY);
  write(fd, "preserve_me", 11);
  close(fd);

  taint_myself();
  int bad_fd = open("/t18_clean.txt", O_RDONLY | O_TRUNC);
  CHECK("T18", "tainted open with O_TRUNC on clean file refused", bad_fd < 0);

  // In clean child, verify file contents preserved
  int pid = fork();
  if (pid == 0) {
    fd = open("/t18_clean.txt", O_RDONLY);
    char buf[16];
    int n = read(fd, buf, 15);
    close(fd);
    exit(n == 11 ? 0 : 1);
  } else {
    int status;
    wait(&status);
    CHECK("T18", "contents unchanged after denied O_TRUNC open", status == 0);
  }
}

// T19: tainted open with O_RDWR refused
void
test_t19_open_rdwr_refused(void)
{
  int fd = open("/t19_clean.txt", O_CREATE | O_WRONLY);
  write(fd, "data", 4);
  close(fd);

  taint_myself();
  int bad_fd = open("/t19_clean.txt", O_RDWR);
  CHECK("T19", "tainted open with O_RDWR on clean file refused", bad_fd < 0);
}

// T13: Inode reuse check using fstat
void
test_t13_inode_reuse(void)
{
  int fd = open("/r.txt", O_CREATE | O_WRONLY);
  if (fd < 0) {
    INCONCLUSIVE("T13", "create /r.txt failed");
    return;
  }
  struct stat st;
  fstat(fd, &st);
  uint target_ino = st.ino;
  write(fd, "test", 4);
  close(fd);

  marktaint("/r.txt");
  unlink("/r.txt");

  // Allocate new files and see if any reuses target_ino
  int found_reuse = 0;
  for (int i = 0; i < 8; i++) {
    char name[16];
    name[0] = '/'; name[1] = 'n'; name[2] = '0' + i; name[3] = '.'; name[4] = 't'; name[5] = 0;
    int nfd = open(name, O_CREATE | O_WRONLY);
    if (nfd >= 0) {
      struct stat nst;
      fstat(nfd, &nst);
      write(nfd, "abc", 3);
      close(nfd);
      if (nst.ino == target_ino) {
        found_reuse = 1;
        // Verify this reused inode is clean
        int cpid = fork();
        if (cpid == 0) {
          int rfd = open(name, O_RDONLY);
          char b[4];
          read(rfd, b, 3);
          close(rfd);
          exit(istainted() == 0 ? 0 : 1);
        } else {
          int s;
          wait(&s);
          CHECK("T13", "reused inode is initialized clean", s == 0);
        }
        break;
      }
    }
  }

  if (!found_reuse) {
    INCONCLUSIVE("T13", "inode reuse not matched within 8 files");
  }
}

int
main(int argc, char *argv[])
{
  run_in_child(test_t5_create_born_untrusted);
  run_in_child(test_t9_unlink_refused);
  run_in_child(test_t11b_open_write_refused);
  run_in_child(test_t16_marktaint_validation);
  run_in_child(test_t18_open_trunc_refused);
  run_in_child(test_t19_open_rdwr_refused);
  run_in_child(test_t13_inode_reuse);
  tlib_summary();
  exit(0);
}
