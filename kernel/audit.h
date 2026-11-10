#ifndef _AUDIT_H_
#define _AUDIT_H_

#include "types.h"

// Audit event categories (Mini Kernel Security Audit Trail)
#define AUDIT_PROCESS_TAINTED   1  // Process became tainted after reading untrusted file
#define AUDIT_FORK_PROPAGATION  2  // Taint inherited by child via fork
#define AUDIT_PIPE_PROPAGATION  3  // Taint transferred to reader via pipe
#define AUDIT_UNLINK_DENIED     4  // Tainted process attempted prohibited unlink
#define AUDIT_EXEC_DENIED       5  // Tainted process attempted prohibited exec
#define AUDIT_WRITE_DOWN_DENIED 6  // Tainted process attempted write to clean file

#define AUDIT_BUF_SIZE 64          // 64 events ring buffer (bounded memory)

struct taint_event {
  uint seq;          // Monotonically increasing sequence number (protected by audit_lock)
  int event_type;    // AUDIT_* event category
  int pid;           // Target process PID
  int related_pid;   // Parent PID (fork) or Writer PID (pipe), or 0
  uint inum;         // Inode number of untrusted file / target file
  int reason;        // Status or rule code (0 = standard denial/propagate)
};

#endif // _AUDIT_H_
