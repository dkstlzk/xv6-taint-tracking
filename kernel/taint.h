#ifndef _TAINT_H_
#define _TAINT_H_

// Centralized taint containment directory and canonical refusal messages.
// Changing TAINT_SAFE_DIR or refusal templates updates kernel enforcement
// and guest test assertions together.
#define TAINT_SAFE_DIR      "/safe"

#define TAINT_REFUSE_EXEC   "taint: exec refused for pid %d\n"
#define TAINT_REFUSE_WRITE  "taint: write refused for pid %d\n"
#define TAINT_REFUSE_LINK   "taint: link refused for pid %d\n"
#define TAINT_REFUSE_UNLINK "taint: unlink refused for pid %d\n"

#endif // _TAINT_H_

