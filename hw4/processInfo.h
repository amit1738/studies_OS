// processInfo.h - Structure for passing process info between user and kernel
//
// This header defines the processInfo structure used by getProcInfo syscall.
// The structure is shared between userspace and kernel.

#ifndef PROCESSINFO_H
#define PROCESSINFO_H

struct processInfo {
    int state;      // process state (UNUSED=0, EMBRYO=1, SLEEPING=2, RUNNABLE=3, RUNNING=4, ZOMBIE=5)
    int ppid;       // parent process ID (0 for init process)
    int sz;         // size of process memory in bytes
    int nfd;        // number of open file descriptors
    int nrswitch;   // number of times process was context switched in
};

#endif // PROCESSINFO_H
