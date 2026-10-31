#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "stat.h"
#include "fs.h"
#include "file.h"
#include "defs.h"

// in_safe checks if the given inode is a direct child of /safe directory.
// Preconditions: caller holds NO lock on ip.
int
in_safe(struct inode *ip)
{
  struct inode *dp;
  struct dirent de;
  uint off;
  int found = 0;

  if ((dp = namei("/safe")) == 0)
    return 0; // no /safe: nothing is safe

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
