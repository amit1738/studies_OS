// sysproc.c - System calls related to processes
//
// Modified to include wrappers for the three new HW4 system calls.

#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "processInfo.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// ============================================================
// HW4: New system call wrappers
// ============================================================

// sys_getNumProc - Wrapper for getNumProc
// No arguments needed, just call the implementation
int
sys_getNumProc(void)
{
  return getNumProc();
}

// sys_getMaxPid - Wrapper for getMaxPid
// No arguments needed, just call the implementation
int
sys_getMaxPid(void)
{
  return getMaxPid();
}

// sys_getProcInfo - Wrapper for getProcInfo
// Arguments: pid (int), pinfo (pointer to processInfo)
int
sys_getProcInfo(void)
{
  int pid;
  struct processInfo *pinfo;

  // Get the first argument (pid)
  if(argint(0, &pid) < 0)
    return -1;

  // Get the second argument (pointer to processInfo)
  // The size is sizeof(struct processInfo)
  if(argptr(1, (char**)&pinfo, sizeof(struct processInfo)) < 0)
    return -1;

  // Call the actual implementation
  return getProcInfo(pid, pinfo);
}
