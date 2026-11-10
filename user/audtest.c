#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/audit.h"
#include "user/user.h"
#include "user/tlib.h"

// user/audtest.c — Comprehensive Security Audit Trail Verification Suite
// Tests:
// 1. Audit retrieval baseline
// 2. PROCESS_TAINTED event logged on untrusted read (first transition)
// 3. FORK_PROPAGATION event logged on fork of tainted parent
// 4. PIPE_PROPAGATION event logged strictly when a clean reader absorbs taint
// 5. UNLINK_DENIED event logged on refused deletion
// 6. EXEC_DENIED event logged on refused execution
// 7. WRITE_DOWN_DENIED event logged on write to clean file
// 8. Buffer overflow (more than 64 events) preserves chronological order and sequence continuity
// 9. Audit log retrieval does not clear or alter process taint state

static int
has_event(struct taint_event *events, int n, int ev_type, int target_pid)
{
  for (int i = 0; i < n; i++) {
    if (events[i].event_type == ev_type) {
      if (target_pid == 0 || events[i].pid == target_pid)
        return 1;
    }
  }
  return 0;
}

void
test_audit_baseline(void)
{
  struct taint_event events[AUDIT_BUF_SIZE];
  int n = gettaintlog(events, AUDIT_BUF_SIZE);
  CHECK("AUD1", "gettaintlog syscall succeeds and returns non-negative count", n >= 0);
}

void
test_audit_safety(void)
{
  int before = istainted();
  struct taint_event events[AUDIT_BUF_SIZE];
  int n = gettaintlog(events, AUDIT_BUF_SIZE);
  int after = istainted();
  CHECK("AUD2", "gettaintlog does not alter process taint state", before == after && n >= 0);
}

void
test_audit_process_tainted(void)
{
  int fd = open("/aud_untrusted.txt", O_CREATE | O_WRONLY);
  if (fd >= 0) {
    write(fd, "payload", 7);
    close(fd);
    marktaint("/aud_untrusted.txt");
    fd = open("/aud_untrusted.txt", O_RDONLY);
    if (fd >= 0) {
      char buf[8];
      read(fd, buf, 7);
      close(fd);
    }
  }

  struct taint_event events[AUDIT_BUF_SIZE];
  int n = gettaintlog(events, AUDIT_BUF_SIZE);
  int my_pid = getpid();
  CHECK("AUD3", "PROCESS_TAINTED event recorded in audit buffer", has_event(events, n, AUDIT_PROCESS_TAINTED, my_pid));
}

void
test_audit_fork_propagation(void)
{
  int child_pid = fork();
  if (child_pid == 0) {
    exit(0);
  }
  wait(0);

  struct taint_event events[AUDIT_BUF_SIZE];
  int n = gettaintlog(events, AUDIT_BUF_SIZE);
  CHECK("AUD4", "FORK_PROPAGATION event recorded in audit buffer", has_event(events, n, AUDIT_FORK_PROPAGATION, child_pid));
}

void
test_audit_unlink_denied(void)
{
  // Current process is tainted; attempt unlink on clean file
  int res = unlink("/clean_aud.txt");
  CHECK("AUD5_REFUSAL", "unlink refused by kernel for tainted process", res == -1);

  struct taint_event events[AUDIT_BUF_SIZE];
  int n = gettaintlog(events, AUDIT_BUF_SIZE);
  int my_pid = getpid();
  CHECK("AUD5", "UNLINK_DENIED event recorded in audit buffer", has_event(events, n, AUDIT_UNLINK_DENIED, my_pid));
}

void
test_audit_exec_denied(void)
{
  // Current process is tainted; attempt exec on binary outside /safe (e.g. echo or ls)
  char *argv[] = { "echo", "test", 0 };
  int cpid = fork();
  if (cpid == 0) {
    exec("echo", argv);
    // If exec succeeds, it shouldn't have; exit 0
    exit(0);
  }
  wait(0);

  struct taint_event events[AUDIT_BUF_SIZE];
  int n = gettaintlog(events, AUDIT_BUF_SIZE);
  CHECK("AUD6", "EXEC_DENIED event recorded in audit buffer", has_event(events, n, AUDIT_EXEC_DENIED, cpid));
}

void
test_audit_write_down_denied(void)
{
  // Current process is tainted; attempt write to existing clean file (README)
  int fd = open("README", O_WRONLY);
  if (fd >= 0) {
    write(fd, "overwrite", 9);
    close(fd);
  }

  struct taint_event events[AUDIT_BUF_SIZE];
  int n = gettaintlog(events, AUDIT_BUF_SIZE);
  int my_pid = getpid();
  CHECK("AUD7", "WRITE_DOWN_DENIED event recorded in audit buffer", has_event(events, n, AUDIT_WRITE_DOWN_DENIED, my_pid));
}

void
test_audit_pipe_propagation(void)
{
  int pfd[2];
  if (pipe(pfd) < 0) {
    CHECK("AUD8", "pipe creation failed", 0);
    return;
  }

  // 1. Ensure parent is clean initially
  if (istainted() != 0) {
    CHECK("AUD8_CLEAN_INIT", "Parent process is clean before pipe test", 0);
    close(pfd[0]);
    close(pfd[1]);
    return;
  }

  // 2. Fork clean child
  int cpid = fork();
  if (cpid < 0) {
    CHECK("AUD8_FORK", "fork failed", 0);
    close(pfd[0]);
    close(pfd[1]);
    return;
  }

  if (cpid == 0) {
    // 3. Child: close write end, verify initially clean, block reading from pipe
    close(pfd[1]);
    if (istainted() != 0) {
      close(pfd[0]);
      exit(2); // Failed: child was tainted at fork
    }

    char buf[16];
    int r = read(pfd[0], buf, sizeof(buf));
    close(pfd[0]);

    if (r <= 0) {
      exit(3); // Failed: pipe read failed
    }

    // Child must now be tainted after reading tainted data
    int child_tainted = istainted();
    exit(child_tainted == 1 ? 0 : 1);
  }

  // 4. Parent: close read end
  close(pfd[0]);

  // 5. Parent taints itself: create file, write, mark tainted, read back
  int fd = open("/aud_pipe_untrusted.txt", O_CREATE | O_WRONLY);
  if (fd >= 0) {
    write(fd, "payload_bytes", 13);
    close(fd);
    marktaint("/aud_pipe_untrusted.txt");

    fd = open("/aud_pipe_untrusted.txt", O_RDONLY);
    if (fd >= 0) {
      char r_buf[16];
      read(fd, r_buf, sizeof(r_buf));
      close(fd);
    }
  }

  int parent_tainted = istainted();
  CHECK("AUD8_PARENT_TAINT", "Parent process successfully tainted by reading untrusted file", parent_tainted == 1);

  // 6. Parent writes tainted data into pipe
  write(pfd[1], "tainted_pipe", 12);
  close(pfd[1]);

  // 7. Wait for child and verify its exit status
  int wstatus = -1;
  wait(&wstatus);
  CHECK("AUD8_CHILD_EXIT", "Child read pipe, verified istainted()==1 and exited with 0", wstatus == 0);

  // 8. Verify audit log contains AUDIT_PIPE_PROPAGATION for child PID
  struct taint_event events[AUDIT_BUF_SIZE];
  int n = gettaintlog(events, AUDIT_BUF_SIZE);
  CHECK("AUD8_EVENT", "AUDIT_PIPE_PROPAGATION event recorded for child PID", has_event(events, n, AUDIT_PIPE_PROPAGATION, cpid));
}

void
test_audit_overflow_and_ordering(void)
{
  // Cause > 70 write-down denials to exceed 64-slot ring buffer
  for (int i = 0; i < 70; i++) {
    unlink("/nonexistent_aud_test.txt");
  }

  struct taint_event events[AUDIT_BUF_SIZE];
  int n = gettaintlog(events, AUDIT_BUF_SIZE);

  CHECK("AUD9_COUNT", "buffer returns exactly 64 events on overflow", n == AUDIT_BUF_SIZE);

  // Check chronological sequence monotonicity: seq[i] < seq[i+1]
  int monotonic = 1;
  for (int i = 0; i < n - 1; i++) {
    if (events[i].seq >= events[i + 1].seq) {
      monotonic = 0;
      break;
    }
  }
  CHECK("AUD9_MONOTONIC", "returned events are strictly ordered by sequence number", monotonic == 1);
}

int
main(int argc, char *argv[])
{
  printf("=== xv6 Security Audit Trail Comprehensive Test Suite ===\n");

  test_audit_baseline();
  test_audit_safety();
  test_audit_pipe_propagation();
  test_audit_process_tainted();
  test_audit_fork_propagation();
  test_audit_unlink_denied();
  test_audit_exec_denied();
  test_audit_write_down_denied();
  test_audit_overflow_and_ordering();

  printf("SUMMARY pass=%d fail=%d inconclusive=%d\n", g_pass, g_fail, g_inconc);
  exit(g_fail > 0 ? 1 : 0);
}
