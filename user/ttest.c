#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/riscv.h"
#include "kernel/fs.h"
#include "user/user.h"
#include "user/tlib.h"

// user/ttest.c — Process and syscall taint test harness (Member B)
// Tests: T0 (hello), T1 (fresh clean), T3 (read latch + zero/invalid/partial boundaries),
//        T4 (fork taint inheritance), T11a (write refusal on pre-opened clean FD)

#define TEST_SMALL_BUF           16
#define TEST_PAGE_SIZE           PGSIZE
#define TEST_BLOCK_SIZE          BSIZE
#define TEST_PARTIAL_PAGE_BYTES  32
#define TEST_BLOCK_OVERLAP       256
#define TEST_PARTIAL_BLK_BYTES   (TEST_BLOCK_SIZE + TEST_BLOCK_OVERLAP)
#define TEST_MULTI_BLK_BYTES     (2 * TEST_BLOCK_SIZE)
#define TEST_WRITE_CHUNK         128

static void
create_untrusted_file(const char *path, char fill_a, char fill_b, int half_len)
{
  char chunk[TEST_WRITE_CHUNK];
  unlink(path);
  int fd = open(path, O_CREATE | O_WRONLY | O_TRUNC);
  if (fd < 0) {
    printf("ttest: failed to create %s\n", path);
    exit(1);
  }
  int remaining = half_len;
  memset(chunk, fill_a, sizeof(chunk));
  while (remaining > 0) {
    int n = remaining > TEST_WRITE_CHUNK ? TEST_WRITE_CHUNK : remaining;
    if (write(fd, chunk, n) != n) {
      printf("ttest: write failed for %s\n", path);
      exit(1);
    }
    remaining -= n;
  }
  remaining = half_len;
  memset(chunk, fill_b, sizeof(chunk));
  while (remaining > 0) {
    int n = remaining > TEST_WRITE_CHUNK ? TEST_WRITE_CHUNK : remaining;
    if (write(fd, chunk, n) != n) {
      printf("ttest: write failed for %s\n", path);
      exit(1);
    }
    remaining -= n;
  }
  close(fd);
  if (marktaint((char *)path) < 0) {
    printf("ttest: marktaint failed for %s\n", path);
    exit(1);
  }
}

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
  int fd = open("/ut_t4.txt", O_CREATE | O_WRONLY | O_TRUNC);
  if (fd >= 0) {
    write(fd, "bad", 3);
    close(fd);
    marktaint("/ut_t4.txt");
    fd = open("/ut_t4.txt", O_RDONLY);
    if (fd >= 0) {
      char buf[TEST_SMALL_BUF];
      read(fd, buf, 3);
      close(fd);
    }
  }

  int parent_taint = istainted();
  if (!parent_taint) {
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
  int fd = open("/ut_t3.txt", O_CREATE | O_WRONLY | O_TRUNC);
  if (fd < 0) {
    CHECK("T3", "create test file", 0);
    return;
  }
  write(fd, "test", 4);
  close(fd);

  if (marktaint("/ut_t3.txt") < 0) {
    CHECK("T3", "marktaint failed", 0);
    return;
  }

  // 1. Zero-byte read must NOT taint
  fd = open("/ut_t3.txt", O_RDONLY);
  if (fd < 0) {
    CHECK("T3", "open untrusted file", 0);
    return;
  }
  char buf[TEST_SMALL_BUF];
  int r0 = read(fd, buf, 0);
  int t0 = istainted();
  close(fd);

  // 2. Positive-byte read MUST taint
  fd = open("/ut_t3.txt", O_RDONLY);
  if (fd < 0) {
    CHECK("T3", "open untrusted file again", 0);
    return;
  }
  int n = read(fd, buf, 2);
  int t1 = istainted();
  close(fd);

  CHECK("T3", "positive-byte read taints while zero-byte read does not",
        r0 == 0 && t0 == 0 && n == 2 && t1 == 1);
}

void
test_t3b_invalid_dest_stays_clean(void)
{
  create_untrusted_file("/ut_inv.txt", 'X', 'X', TEST_PARTIAL_PAGE_BYTES);

  int ufd = open("/ut_inv.txt", O_RDONLY);
  if (ufd < 0) {
    CHECK("T3b", "open untrusted file", 0);
    return;
  }

  char *end = sbrk(0);
  int r_zero = read(ufd, (void *)0, TEST_SMALL_BUF);
  int t_zero = istainted();

  int r_max = read(ufd, (void *)~(uint64)0, TEST_SMALL_BUF);
  int t_max = istainted();

  int r_end = read(ufd, end, TEST_SMALL_BUF);
  int t_end = istainted();
  close(ufd);

  // Clean process must still be able to write to a clean file
  unlink("/cl_inv.txt");
  int cfd = open("/cl_inv.txt", O_CREATE | O_WRONLY | O_TRUNC);
  int w_clean = (cfd >= 0) ? write(cfd, "ok", 2) : -1;
  if (cfd >= 0)
    close(cfd);

  CHECK("T3b", "invalid zero-transfer read destinations leave clean process clean",
        r_zero == -1 && t_zero == 0 &&
        r_max == -1 && t_max == 0 &&
        r_end == -1 && t_end == 0 &&
        w_clean == 2);
}

void
test_t3c_partial_page_boundary_taints(void)
{
  create_untrusted_file("/ut_pg.txt", 'P', 'P', TEST_PARTIAL_PAGE_BYTES);

  unlink("/cl_pg.txt");
  int clean_fd = open("/cl_pg.txt", O_CREATE | O_WRONLY | O_TRUNC);
  if (clean_fd < 0) {
    CHECK("T3c", "create clean target file", 0);
    return;
  }

  char *page = sbrk(TEST_PAGE_SIZE);
  if (page == SBRK_ERROR) {
    close(clean_fd);
    CHECK("T3c", "allocate test page", 0);
    return;
  }
  char *end = sbrk(0);
  char *dst = end - TEST_PARTIAL_PAGE_BYTES;
  memset(dst, 0, TEST_PARTIAL_PAGE_BYTES);

  int ufd = open("/ut_pg.txt", O_RDONLY);
  if (ufd < 0) {
    close(clean_fd);
    CHECK("T3c", "open untrusted file", 0);
    return;
  }

  // Copies TEST_PARTIAL_PAGE_BYTES into valid page, then faults at `end`.
  int r = read(ufd, dst, 2 * TEST_PARTIAL_PAGE_BYTES);
  close(ufd);

  int bytes_reached = (dst[0] == 'P' && dst[TEST_PARTIAL_PAGE_BYTES - 1] == 'P');
  int t = istainted();
  int w_refused = write(clean_fd, "leak", 4);
  close(clean_fd);

  CHECK("T3c", "partial read failing at page boundary taints process and refuses clean write",
        r == -1 && bytes_reached && t == 1 && w_refused == -1);
}

void
test_t3d_partial_block_and_page_boundary_taints(void)
{
  // File spans two filesystem blocks: [0..BSIZE) = 'A', [BSIZE..2*BSIZE) = 'B'
  create_untrusted_file("/ut_blk.txt", 'A', 'B', TEST_BLOCK_SIZE);

  unlink("/cl_blk.txt");
  int clean_fd = open("/cl_blk.txt", O_CREATE | O_WRONLY | O_TRUNC);
  if (clean_fd < 0) {
    CHECK("T3d", "create clean target file", 0);
    return;
  }

  char *page = sbrk(TEST_PAGE_SIZE);
  if (page == SBRK_ERROR) {
    close(clean_fd);
    CHECK("T3d", "allocate test page", 0);
    return;
  }
  char *end = sbrk(0);
  char *dst = end - TEST_PARTIAL_BLK_BYTES;
  memset(dst, 0, TEST_PARTIAL_BLK_BYTES);

  int ufd = open("/ut_blk.txt", O_RDONLY);
  if (ufd < 0) {
    close(clean_fd);
    CHECK("T3d", "open multi-block untrusted file", 0);
    return;
  }

  // Block 0 (1024 B) succeeds; Block 1 copies 256 B then faults at `end`.
  int r = read(ufd, dst, TEST_MULTI_BLK_BYTES);
  close(ufd);

  int bytes_reached = (dst[0] == 'A' &&
                       dst[TEST_BLOCK_SIZE - 1] == 'A' &&
                       dst[TEST_BLOCK_SIZE] == 'B' &&
                       dst[TEST_PARTIAL_BLK_BYTES - 1] == 'B');
  int t = istainted();
  int w_refused = write(clean_fd, "leak", 4);
  close(clean_fd);

  CHECK("T3d", "partial read across FS-block and page boundary taints and refuses clean write",
        r == -1 && bytes_reached && t == 1 && w_refused == -1);
}

void
test_t11a_write_clean_fd_after_taint(void)
{
  int clean_fd = open("/clean_t11a.txt", O_CREATE | O_WRONLY | O_TRUNC);
  if (clean_fd < 0) {
    CHECK("T11a", "create clean file", 0);
    return;
  }

  int ufd = open("/ut_t11a.txt", O_CREATE | O_WRONLY | O_TRUNC);
  write(ufd, "taint", 5);
  close(ufd);
  marktaint("/ut_t11a.txt");

  ufd = open("/ut_t11a.txt", O_RDONLY);
  char buf[TEST_SMALL_BUF];
  read(ufd, buf, 5);
  close(ufd);

  int t = istainted();
  if (t != 1) {
    close(clean_fd);
    CHECK("T11a", "process became tainted", 0);
    return;
  }

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
  run_in_child(test_t3b_invalid_dest_stays_clean);
  run_in_child(test_t3c_partial_page_boundary_taints);
  run_in_child(test_t3d_partial_block_and_page_boundary_taints);
  run_in_child(test_t11a_write_clean_fd_after_taint);
  tlib_summary();
  exit(0);
}
