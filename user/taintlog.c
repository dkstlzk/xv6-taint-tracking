#include "kernel/types.h"
#include "kernel/audit.h"
#include "user/user.h"

static const char *
event_name(int ev)
{
  switch (ev) {
  case AUDIT_PROCESS_TAINTED:
    return "PROCESS_TAINTED";
  case AUDIT_FORK_PROPAGATION:
    return "FORK_PROPAGATION";
  case AUDIT_PIPE_PROPAGATION:
    return "PIPE_PROPAGATION";
  case AUDIT_UNLINK_DENIED:
    return "UNLINK_DENIED";
  case AUDIT_EXEC_DENIED:
    return "EXEC_DENIED";
  case AUDIT_WRITE_DOWN_DENIED:
    return "WRITE_DOWN_DENIED";
  default:
    return "UNKNOWN_EVENT";
  }
}

int
main(int argc, char *argv[])
{
  struct taint_event events[AUDIT_BUF_SIZE];
  int n = gettaintlog(events, AUDIT_BUF_SIZE);

  if (n < 0) {
    fprintf(2, "taintlog: error retrieving audit log from kernel\n");
    exit(1);
  }

  printf("=== xv6 Kernel Security Audit Trail (%d events) ===\n", n);
  for (int i = 0; i < n; i++) {
    printf("[%d] SEQ=%u %s PID=%d", i + 1, events[i].seq, event_name(events[i].event_type), events[i].pid);
    if (events[i].related_pid > 0) {
      printf(" RELATED_PID=%d", events[i].related_pid);
    }
    if (events[i].inum > 0) {
      printf(" INODE=%u", events[i].inum);
    }
    printf("\n");
  }
  exit(0);
}
