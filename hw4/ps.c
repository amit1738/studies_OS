// ps.c - User-space program to display process information
//
// This program is similar to the Linux ps command.
// It displays all active processes with their information using the new system calls.

#include "types.h"
#include "stat.h"
#include "user.h"

// State names matching the enum in proc.h
// UNUSED=0, EMBRYO=1, SLEEPING=2, RUNNABLE=3, RUNNING=4, ZOMBIE=5
char *stateNames[] = {
    "unused",    // 0 - should not appear in output
    "embryo",    // 1
    "sleeping",  // 2
    "runnable",  // 3
    "running",   // 4
    "zombie"     // 5
};

// Simple bubble sort to sort PIDs in ascending order
void sortPids(int *pids, int n) {
    int i, j, temp;
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - i - 1; j++) {
            if (pids[j] > pids[j + 1]) {
                temp = pids[j];
                pids[j] = pids[j + 1];
                pids[j + 1] = temp;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int numProc, maxPid;
    struct processInfo pinfo;
    int pids[64];  // Array to store found PIDs
    int pidCount = 0;
    int pid;
    int i;
    char *stateName;
    
    // Get the total number of active processes
    numProc = getNumProc();
    printf(1, "Total number of active processes: %d\n", numProc);
    
    // Get the maximum PID
    maxPid = getMaxPid();
    printf(1, "Maximum PID: %d\n", maxPid);
    
    // Print the header line
    printf(1, "PID\tSTATE\t\tPPID\tSZ\tNFD\tNRSWITCH\n");
    
    // Find all active processes by iterating through possible PIDs
    // Start from 1 (init) up to maxPid
    for (pid = 1; pid <= maxPid; pid++) {
        if (getProcInfo(pid, &pinfo) == 0) {
            // Process exists, add to list
            pids[pidCount++] = pid;
        }
    }
    
    // Sort PIDs in ascending order
    sortPids(pids, pidCount);
    
    // Print information for each process
    for (i = 0; i < pidCount; i++) {
        pid = pids[i];
        if (getProcInfo(pid, &pinfo) == 0) {
            // Get state name
            if (pinfo.state >= 1 && pinfo.state <= 5) {
                stateName = stateNames[pinfo.state];
            } else {
                stateName = "unknown";
            }
            
            // Print process info with proper alignment
            printf(1, "%d\t%s\t\t%d\t%d\t%d\t%d\n",
                   pid,
                   stateName,
                   pinfo.ppid,
                   pinfo.sz,
                   pinfo.nfd,
                   pinfo.nrswitch);
        }
    }
    
    exit();
}
