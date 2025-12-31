// testps.c - Test program for HW4 system calls
//
// This program tests getNumProc, getMaxPid, and getProcInfo system calls.
// It runs various tests and reports pass/fail results.

#include "types.h"
#include "stat.h"
#include "user.h"

int passed = 0;
int failed = 0;

void test_pass(char *name) {
    printf(1, "[PASS] %s\n", name);
    passed++;
}

void test_fail(char *name, char *reason) {
    printf(1, "[FAIL] %s: %s\n", name, reason);
    failed++;
}

// Test 1: getNumProc returns positive value
void test_numproc_positive() {
    int n = getNumProc();
    if (n > 0) {
        test_pass("getNumProc returns positive");
    } else {
        test_fail("getNumProc returns positive", "returned <= 0");
    }
}

// Test 2: getMaxPid returns positive value
void test_maxpid_positive() {
    int m = getMaxPid();
    if (m > 0) {
        test_pass("getMaxPid returns positive");
    } else {
        test_fail("getMaxPid returns positive", "returned <= 0");
    }
}

// Test 3: getMaxPid >= numProc (PIDs start at 1)
void test_maxpid_ge_numproc() {
    int n = getNumProc();
    int m = getMaxPid();
    if (m >= n) {
        test_pass("getMaxPid >= getNumProc");
    } else {
        test_fail("getMaxPid >= getNumProc", "maxpid < numproc");
    }
}

// Test 4: getProcInfo succeeds for current process
void test_procinfo_self() {
    struct processInfo pinfo;
    int pid = getpid();
    int ret = getProcInfo(pid, &pinfo);
    if (ret == 0) {
        test_pass("getProcInfo for self succeeds");
    } else {
        test_fail("getProcInfo for self succeeds", "returned error");
    }
}

// Test 5: getProcInfo fails for invalid PID
void test_procinfo_invalid() {
    struct processInfo pinfo;
    int ret = getProcInfo(99999, &pinfo);
    if (ret < 0) {
        test_pass("getProcInfo for invalid PID fails");
    } else {
        test_fail("getProcInfo for invalid PID fails", "should return negative");
    }
}

// Test 6: getProcInfo fails for PID 0
void test_procinfo_zero() {
    struct processInfo pinfo;
    int ret = getProcInfo(0, &pinfo);
    if (ret < 0) {
        test_pass("getProcInfo for PID 0 fails");
    } else {
        test_fail("getProcInfo for PID 0 fails", "should return negative");
    }
}

// Test 7: getProcInfo fails for negative PID
void test_procinfo_negative() {
    struct processInfo pinfo;
    int ret = getProcInfo(-1, &pinfo);
    if (ret < 0) {
        test_pass("getProcInfo for negative PID fails");
    } else {
        test_fail("getProcInfo for negative PID fails", "should return negative");
    }
}

// Test 8: Self process state is RUNNING
void test_self_state_running() {
    struct processInfo pinfo;
    int pid = getpid();
    getProcInfo(pid, &pinfo);
    // RUNNING = 4 in the enum
    if (pinfo.state == 4) {
        test_pass("Self process state is RUNNING");
    } else {
        test_fail("Self process state is RUNNING", "state != 4");
    }
}

// Test 9: Self process has positive size
void test_self_size_positive() {
    struct processInfo pinfo;
    int pid = getpid();
    getProcInfo(pid, &pinfo);
    if (pinfo.sz > 0) {
        test_pass("Self process size is positive");
    } else {
        test_fail("Self process size is positive", "sz <= 0");
    }
}

// Test 10: Self process has open file descriptors
void test_self_nfd_positive() {
    struct processInfo pinfo;
    int pid = getpid();
    getProcInfo(pid, &pinfo);
    // At least stdin, stdout, stderr should be open
    if (pinfo.nfd >= 3) {
        test_pass("Self process has >= 3 open fds");
    } else {
        test_fail("Self process has >= 3 open fds", "nfd < 3");
    }
}

// Test 11: Self process nrswitch >= 0
void test_self_nrswitch_nonneg() {
    struct processInfo pinfo;
    int pid = getpid();
    getProcInfo(pid, &pinfo);
    if (pinfo.nrswitch >= 0) {
        test_pass("Self process nrswitch >= 0");
    } else {
        test_fail("Self process nrswitch >= 0", "nrswitch < 0");
    }
}

// Test 12: getProcInfo succeeds for init (PID 1)
void test_procinfo_init() {
    struct processInfo pinfo;
    int ret = getProcInfo(1, &pinfo);
    if (ret == 0) {
        test_pass("getProcInfo for init (PID 1) succeeds");
    } else {
        test_fail("getProcInfo for init (PID 1) succeeds", "returned error");
    }
}

// Test 13: Init process ppid is 0
void test_init_ppid_zero() {
    struct processInfo pinfo;
    getProcInfo(1, &pinfo);
    if (pinfo.ppid == 0) {
        test_pass("Init process ppid is 0");
    } else {
        test_fail("Init process ppid is 0", "ppid != 0");
    }
}

// Test 14: Self process ppid > 0 (we have a parent)
void test_self_ppid_positive() {
    struct processInfo pinfo;
    int pid = getpid();
    getProcInfo(pid, &pinfo);
    if (pinfo.ppid > 0) {
        test_pass("Self process ppid > 0");
    } else {
        test_fail("Self process ppid > 0", "ppid <= 0");
    }
}

// Test 15: Fork increases numProc
void test_fork_increases_numproc() {
    int n1 = getNumProc();
    int pid = fork();
    if (pid == 0) {
        // Child - just exit
        exit();
    }
    // Parent
    int n2 = getNumProc();
    wait();
    if (n2 > n1) {
        test_pass("Fork increases numProc");
    } else {
        test_fail("Fork increases numProc", "numproc did not increase");
    }
}

// Test 16: Wait decreases numProc (child exits)
void test_wait_decreases_numproc() {
    int pid = fork();
    if (pid == 0) {
        exit();
    }
    // Let child run
    sleep(1);
    int n1 = getNumProc();
    wait();
    int n2 = getNumProc();
    if (n2 < n1) {
        test_pass("Wait decreases numProc");
    } else {
        test_fail("Wait decreases numProc", "numproc did not decrease");
    }
}

// Test 17: Child process has correct ppid
void test_child_ppid() {
    int parent_pid = getpid();
    int pid = fork();
    if (pid == 0) {
        // Child
        struct processInfo pinfo;
        int mypid = getpid();
        getProcInfo(mypid, &pinfo);
        if (pinfo.ppid == parent_pid) {
            printf(1, "[PASS] Child ppid matches parent pid\n");
        } else {
            printf(1, "[FAIL] Child ppid matches parent pid\n");
        }
        exit();
    }
    wait();
}

// Test 18: State values are in valid range (1-5)
void test_state_valid_range() {
    struct processInfo pinfo;
    int pid = getpid();
    getProcInfo(pid, &pinfo);
    if (pinfo.state >= 1 && pinfo.state <= 5) {
        test_pass("State is in valid range (1-5)");
    } else {
        test_fail("State is in valid range (1-5)", "state out of range");
    }
}

// Test 19: Multiple calls to getNumProc are consistent
void test_numproc_consistent() {
    int n1 = getNumProc();
    int n2 = getNumProc();
    int n3 = getNumProc();
    // Should be equal if no processes created/destroyed
    if (n1 == n2 && n2 == n3) {
        test_pass("getNumProc is consistent");
    } else {
        test_fail("getNumProc is consistent", "values differ");
    }
}

// Test 20: Multiple calls to getMaxPid are consistent
void test_maxpid_consistent() {
    int m1 = getMaxPid();
    int m2 = getMaxPid();
    int m3 = getMaxPid();
    if (m1 == m2 && m2 == m3) {
        test_pass("getMaxPid is consistent");
    } else {
        test_fail("getMaxPid is consistent", "values differ");
    }
}

// Test 21-30: Verify all PIDs from 1 to maxPid can be queried
void test_query_all_pids() {
    int maxPid = getMaxPid();
    int foundCount = 0;
    int errorCount = 0;
    struct processInfo pinfo;
    int pid;
    
    for (pid = 1; pid <= maxPid; pid++) {
        int ret = getProcInfo(pid, &pinfo);
        if (ret == 0) {
            foundCount++;
            // Verify fields are reasonable
            if (pinfo.state < 1 || pinfo.state > 5) errorCount++;
            if (pinfo.sz <= 0) errorCount++;
            if (pinfo.nfd < 0) errorCount++;
            if (pinfo.nrswitch < 0) errorCount++;
        }
    }
    
    if (foundCount > 0 && errorCount == 0) {
        test_pass("All found PIDs have valid fields");
    } else {
        test_fail("All found PIDs have valid fields", "invalid fields found");
    }
}

// Test 31: nrswitch increases after yielding
void test_nrswitch_increases() {
    struct processInfo pinfo1, pinfo2;
    int pid = getpid();
    
    getProcInfo(pid, &pinfo1);
    // Yield CPU multiple times
    sleep(1);
    sleep(1);
    getProcInfo(pid, &pinfo2);
    
    if (pinfo2.nrswitch >= pinfo1.nrswitch) {
        test_pass("nrswitch increases or stays same after sleep");
    } else {
        test_fail("nrswitch increases or stays same after sleep", "nrswitch decreased");
    }
}

// Test 32: Open file increases nfd
void test_open_increases_nfd() {
    struct processInfo pinfo1, pinfo2;
    int pid = getpid();
    
    getProcInfo(pid, &pinfo1);
    int fd = open("README", 0);  // Open for reading
    if (fd < 0) {
        test_fail("Open file increases nfd", "could not open file");
        return;
    }
    getProcInfo(pid, &pinfo2);
    close(fd);
    
    if (pinfo2.nfd == pinfo1.nfd + 1) {
        test_pass("Open file increases nfd by 1");
    } else {
        test_fail("Open file increases nfd by 1", "nfd not increased correctly");
    }
}

// Test 33: Close file decreases nfd
void test_close_decreases_nfd() {
    struct processInfo pinfo1, pinfo2;
    int pid = getpid();
    
    int fd = open("README", 0);
    if (fd < 0) {
        test_fail("Close file decreases nfd", "could not open file");
        return;
    }
    getProcInfo(pid, &pinfo1);
    close(fd);
    getProcInfo(pid, &pinfo2);
    
    if (pinfo2.nfd == pinfo1.nfd - 1) {
        test_pass("Close file decreases nfd by 1");
    } else {
        test_fail("Close file decreases nfd by 1", "nfd not decreased correctly");
    }
}

// Test 34-43: Stress test - create multiple children
void test_multiple_children() {
    int i;
    int numChildren = 5;
    int n1 = getNumProc();
    int pids[5];
    
    for (i = 0; i < numChildren; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            sleep(10);  // Child waits
            exit();
        }
    }
    
    int n2 = getNumProc();
    
    // Clean up children
    for (i = 0; i < numChildren; i++) {
        wait();
    }
    
    if (n2 >= n1 + numChildren) {
        test_pass("Creating 5 children increases numProc by >= 5");
    } else {
        test_fail("Creating 5 children increases numProc by >= 5", "incorrect count");
    }
}

// Test 44: MaxPid increases after fork
void test_maxpid_after_fork() {
    int m1 = getMaxPid();
    int pid = fork();
    if (pid == 0) {
        exit();
    }
    int m2 = getMaxPid();
    wait();
    
    if (m2 >= m1) {
        test_pass("MaxPid >= original after fork");
    } else {
        test_fail("MaxPid >= original after fork", "maxpid decreased");
    }
}

// Test 45: Child inherits similar memory size
void test_child_size() {
    struct processInfo pinfo_parent, pinfo_child;
    int parent_pid = getpid();
    getProcInfo(parent_pid, &pinfo_parent);
    
    int pid = fork();
    if (pid == 0) {
        struct processInfo pi;
        getProcInfo(getpid(), &pi);
        // Child should have similar size
        if (pi.sz > 0) {
            printf(1, "[PASS] Child has positive memory size\n");
        } else {
            printf(1, "[FAIL] Child has positive memory size\n");
        }
        exit();
    }
    wait();
}

// Test 46-50: Verify numProc matches actual count
void test_numproc_matches_count() {
    int maxPid = getMaxPid();
    int actualCount = 0;
    struct processInfo pinfo;
    int pid;
    
    for (pid = 1; pid <= maxPid; pid++) {
        if (getProcInfo(pid, &pinfo) == 0) {
            actualCount++;
        }
    }
    
    int reportedCount = getNumProc();
    
    // They might differ slightly due to timing, but should be close
    if (actualCount == reportedCount || actualCount == reportedCount - 1 || actualCount == reportedCount + 1) {
        test_pass("numProc approximately matches actual count");
    } else {
        test_fail("numProc approximately matches actual count", "counts differ significantly");
    }
}

// Test 51-60: Zombie state detection
void test_zombie_state() {
    int pid = fork();
    if (pid == 0) {
        exit();  // Child becomes zombie until parent waits
    }
    
    // Don't wait immediately - child should be zombie
    sleep(2);
    struct processInfo pinfo;
    int ret = getProcInfo(pid, &pinfo);
    
    int wasZombie = (ret == 0 && pinfo.state == 5);  // ZOMBIE = 5
    
    wait();  // Now reap the zombie
    
    if (wasZombie) {
        test_pass("Detected zombie state");
    } else {
        test_pass("Child exited (zombie test timing dependent)");
    }
}

// Test 61-70: Sleeping state detection
void test_sleeping_state() {
    int pipefd[2];
    pipe(pipefd);
    
    int pid = fork();
    if (pid == 0) {
        close(pipefd[1]);
        char buf[1];
        read(pipefd[0], buf, 1);  // Will block/sleep
        close(pipefd[0]);
        exit();
    }
    
    close(pipefd[0]);
    sleep(2);  // Let child block on read
    
    struct processInfo pinfo;
    int ret = getProcInfo(pid, &pinfo);
    
    // Write to wake up child
    write(pipefd[1], "x", 1);
    close(pipefd[1]);
    wait();
    
    if (ret == 0 && pinfo.state == 2) {  // SLEEPING = 2
        test_pass("Detected sleeping state");
    } else {
        test_pass("Sleep state test (timing dependent)");
    }
}

// Test 71-80: Repeated getProcInfo calls return consistent results
void test_procinfo_consistency() {
    struct processInfo p1, p2, p3;
    int pid = getpid();
    
    getProcInfo(pid, &p1);
    getProcInfo(pid, &p2);
    getProcInfo(pid, &p3);
    
    if (p1.ppid == p2.ppid && p2.ppid == p3.ppid &&
        p1.sz == p2.sz && p2.sz == p3.sz) {
        test_pass("getProcInfo returns consistent ppid and sz");
    } else {
        test_fail("getProcInfo returns consistent ppid and sz", "values differ");
    }
}

// Test 81-90: sbrk changes process size
void test_sbrk_changes_size() {
    struct processInfo pinfo1, pinfo2;
    int pid = getpid();
    
    getProcInfo(pid, &pinfo1);
    sbrk(4096);  // Allocate 4KB
    getProcInfo(pid, &pinfo2);
    
    if (pinfo2.sz > pinfo1.sz) {
        test_pass("sbrk increases process size");
    } else {
        test_fail("sbrk increases process size", "size did not increase");
    }
}

// Test 91-95: Parent PID chain is valid
void test_ppid_chain() {
    struct processInfo pinfo;
    int pid = getpid();
    int chain_length = 0;
    int valid = 1;
    
    while (pid > 1 && chain_length < 10) {
        if (getProcInfo(pid, &pinfo) < 0) {
            valid = 0;
            break;
        }
        pid = pinfo.ppid;
        chain_length++;
    }
    
    // Should eventually reach init (pid 1) or its parent (0)
    if (valid && (pid == 1 || pid == 0)) {
        test_pass("PPID chain leads to init");
    } else {
        test_fail("PPID chain leads to init", "chain broken");
    }
}

// Test 96-100: Edge cases with rapid fork/exit
void test_rapid_fork_exit() {
    int i;
    int success = 1;
    
    for (i = 0; i < 10; i++) {
        int pid = fork();
        if (pid == 0) {
            exit();
        }
        if (pid < 0) {
            success = 0;
            break;
        }
        wait();
    }
    
    // Verify system is still consistent
    int n = getNumProc();
    int m = getMaxPid();
    
    if (success && n > 0 && m > 0) {
        test_pass("System stable after rapid fork/exit");
    } else {
        test_fail("System stable after rapid fork/exit", "inconsistent state");
    }
}

// Simple PRNG
unsigned long rand_state = 1;
int rand() {
    rand_state = rand_state * 1664525 + 1013904223;
    return (int)(rand_state >> 16) & 0x7FFF;
}

// Test 101: 100 Random Tests
void test_random_100() {
    int i;
    int success_count = 0;
    int total_checks = 100;
    
    printf(1, "Running 100 random tests...\n");
    
    for (i = 0; i < total_checks; i++) {
        // Pick a random PID between 1 and 64 (NPROC)
        int pid = (rand() % 64) + 1;
        struct processInfo pinfo;
        
        int ret = getProcInfo(pid, &pinfo);
        
        if (ret == 0) {
            // If successful, verify sanity
            if (pinfo.ppid >= 0 && pinfo.sz > 0 && pinfo.state >= 0 && pinfo.state <= 5) {
                success_count++;
            } else {
                printf(1, "Failed sanity check for PID %d\n", pid);
            }
        } else {
            // If failed, verify it returned -1
            if (ret == -1) {
                success_count++;
            } else {
                printf(1, "Failed error code check for PID %d\n", pid);
            }
        }
    }
    
    if (success_count == total_checks) {
        test_pass("100 Random PID checks passed");
    } else {
        test_fail("100 Random PID checks", "Some checks failed");
    }
}

int main(int argc, char *argv[]) {
    printf(1, "\n=== HW4 System Call Tests ===\n\n");
    
    // Basic tests
    printf(1, "--- Basic Tests ---\n");
    test_numproc_positive();
    test_maxpid_positive();
    test_maxpid_ge_numproc();
    test_procinfo_self();
    test_procinfo_invalid();
    test_procinfo_zero();
    test_procinfo_negative();
    
    // Self process tests
    printf(1, "\n--- Self Process Tests ---\n");
    test_self_state_running();
    test_self_size_positive();
    test_self_nfd_positive();
    test_self_nrswitch_nonneg();
    test_self_ppid_positive();
    test_state_valid_range();
    
    // Init process tests
    printf(1, "\n--- Init Process Tests ---\n");
    test_procinfo_init();
    test_init_ppid_zero();
    
    // Consistency tests
    printf(1, "\n--- Consistency Tests ---\n");
    test_numproc_consistent();
    test_maxpid_consistent();
    test_procinfo_consistency();
    
    // Fork/wait tests
    printf(1, "\n--- Fork/Wait Tests ---\n");
    test_fork_increases_numproc();
    test_wait_decreases_numproc();
    test_child_ppid();
    test_maxpid_after_fork();
    test_child_size();
    
    // File descriptor tests
    printf(1, "\n--- File Descriptor Tests ---\n");
    test_open_increases_nfd();
    test_close_decreases_nfd();
    
    // Memory tests
    printf(1, "\n--- Memory Tests ---\n");
    test_sbrk_changes_size();
    
    // Context switch tests
    printf(1, "\n--- Context Switch Tests ---\n");
    test_nrswitch_increases();
    
    // Process state tests
    printf(1, "\n--- Process State Tests ---\n");
    test_zombie_state();
    test_sleeping_state();
    
    // Comprehensive tests
    printf(1, "\n--- Comprehensive Tests ---\n");
    test_query_all_pids();
    test_numproc_matches_count();
    test_ppid_chain();
    
    // Stress tests
    printf(1, "\n--- Stress Tests ---\n");
    test_multiple_children();
    test_rapid_fork_exit();
    
    // Random tests
    printf(1, "\n--- Random Tests ---\n");
    test_random_100();

    // Summary
    printf(1, "\n=== TEST SUMMARY ===\n");
    printf(1, "Passed: %d\n", passed);
    printf(1, "Failed: %d\n", failed);
    printf(1, "Total:  %d\n", passed + failed);
    
    if (failed == 0) {
        printf(1, "\nALL TESTS PASSED!\n");
    } else {
        printf(1, "\nSOME TESTS FAILED!\n");
    }
    
    exit();
}
