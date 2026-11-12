#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "stat.h"
#include "fs.h"
#include "file.h"
#include "proc.h"
#include "defs.h"
#include "taint.h"

// in_safe checks if the given inode is a direct child of TAINT_SAFE_DIR (/safe).
// Preconditions: caller holds NO lock on ip.
int
in_safe(struct inode *ip)
{
  struct inode *dp;
  struct dirent de;
  uint off;
  int found = 0;

  if ((dp = namei(TAINT_SAFE_DIR)) == 0)
    return 0; // no safe directory: nothing is safe

  ilock(dp);
  if (dp->type == T_DIR && dp->dev == ip->dev && dp->inum != ip->inum) {
    for (off = 0; off < dp->size; off += sizeof(de)) {
      if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
        panic("in_safe read");
      if (de.inum == 0)
        continue;
      if (strncmp(de.name, ".", DIRSIZ) == 0 || strncmp(de.name, "..", DIRSIZ) == 0)
        continue;
      if (de.inum == ip->inum) {
        found = 1;
        break;
      }
    }
  }
  iunlockput(dp);
  return found;
}

// ip_untrusted checks if inode is untrusted.
// Preconditions: caller holds ip->lock.
int
ip_untrusted(struct inode *ip)
{
  return ip->untrusted != 0;
}

// =====================================================================
// Mini Kernel Security Audit Trail (Extension)
// =====================================================================
#include "audit.h"

static struct spinlock audit_lock;
static struct taint_event audit_buf[AUDIT_BUF_SIZE];
static uint audit_head = 0;  // Next index to write
static uint audit_count = 0; // Total events ever logged

void
audit_init(void)
{
  initlock(&audit_lock, "audit");
  audit_head = 0;
  audit_count = 0;
}

void
audit_log(int event_type, int pid, int related_pid, uint inum, int reason)
{
  acquire(&audit_lock);

  uint idx = audit_head % AUDIT_BUF_SIZE;
  audit_buf[idx].seq = audit_count + 1; // Monotonic 1-based sequence number
  audit_buf[idx].event_type = event_type;
  audit_buf[idx].pid = pid;
  audit_buf[idx].related_pid = related_pid;
  audit_buf[idx].inum = inum;
  audit_buf[idx].reason = reason;

  audit_head = (audit_head + 1) % AUDIT_BUF_SIZE;
  audit_count++;

  release(&audit_lock);
}

// Safely copy out up to max_events recent audit events to user space
int
audit_getlog(uint64 uaddr, int max_events)
{
  if (uaddr == 0 || max_events <= 0)
    return -1;

  struct taint_event tmp[AUDIT_BUF_SIZE];
  int n = 0;

  acquire(&audit_lock);

  // How many valid events are currently in the buffer?
  uint total_in_buf = audit_count < AUDIT_BUF_SIZE ? audit_count : AUDIT_BUF_SIZE;
  if (max_events < total_in_buf)
    n = max_events;
  else
    n = total_in_buf;

  // Copy events chronologically starting from oldest available
  uint start;
  if (audit_count < AUDIT_BUF_SIZE) {
    start = 0;
  } else {
    start = audit_head; // In circular buffer, head is the oldest slot when full
  }

  // If user requested fewer than available, take the most recent n events
  if (n < total_in_buf) {
    start = (start + (total_in_buf - n)) % AUDIT_BUF_SIZE;
  }

  for (int i = 0; i < n; i++) {
    uint src = (start + i) % AUDIT_BUF_SIZE;
    tmp[i] = audit_buf[src];
  }

  release(&audit_lock);

  // Perform copyout to user space OUTSIDE spinlock
  struct proc *p = myproc();
  if (copyout(p->pagetable, p->sz, uaddr, (char *)tmp, n * sizeof(struct taint_event)) < 0)
    return -1;

  return n;
}
