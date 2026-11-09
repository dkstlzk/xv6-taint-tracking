# Mock Viva Questions & Technical Drills (submissions/mock_viva_drills.md)

---

## Member C: Filesystem Persistence & 64-Byte Dinode Layout
1. **Q**: How was `untrusted` added to `struct dinode` without changing its size?
   - **A**: In stock xv6, `struct dinode` had 12 direct block addresses (`NDIRECT = 12`) plus 1 indirect block address. We reduced `NDIRECT` to 11 and added `uint untrusted`. Total size is:
     $$\text{type}(2) + \text{major}(2) + \text{minor}(2) + \text{nlink}(2) + \text{size}(4) + \text{addrs}(11 + 1)\times 4 + \text{untrusted}(4) = 64\text{ bytes}$$
     Because $64\text{ bytes}$ is unchanged, inodes per block ($\text{IPB} = \text{BSIZE}/64 = 16$) is preserved, and `mkfs.c` requires zero code modifications.
2. **Q**: What is the impact of `NDIRECT 11` on maximum file size?
   - **A**: Maximum file size is $(11 + 256) \times 1024 = 267\text{ blocks} = 273,408\text{ bytes}$ (1 block less than stock xv6).
3. **Q**: How does T13 confirm inode reuse?
   - **A**: T13 creates `/r.txt`, retrieves its inode number via `fstat()`, marks it untrusted, and unlinks it. It then creates 8 fresh files. When a newly created file matches the exact original inode number (`old.ino == new.ino`), it verifies that reading the new file leaves the process clean, proving taint was reset upon deallocation.
