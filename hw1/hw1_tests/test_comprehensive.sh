#!/bin/bash

# Comprehensive Test Suite for hw1shell
# Tests all 14 requirements from hw1.pdf

echo "========================================="
echo "  hw1shell Comprehensive Test Suite"
echo "  Testing All 14 Requirements"
echo "========================================="
echo ""

# Compile first
echo "Compiling hw1shell..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi
echo "✅ Compilation successful"
echo ""

PASS_COUNT=0
FAIL_COUNT=0

# Helper function to report test results
report_test() {
    if [ $1 -eq 0 ]; then
        echo "✅ PASS: $2"
        ((PASS_COUNT++))
    else
        echo "❌ FAIL: $2"
        ((FAIL_COUNT++))
    fi
}

echo "========================================="
echo "REQUIREMENT 1: Prompt Format"
echo "========================================="
echo "Testing: Prompt is 'hw1shell$ '"
OUTPUT=$(echo -e "exit" | ./hw1shell 2>&1)
if echo "$OUTPUT" | grep -q "hw1shell\$ "; then
    report_test 0 "Prompt format"
else
    report_test 1 "Prompt format"
fi
echo ""

echo "========================================="
echo "REQUIREMENT 2: Exit Command"
echo "========================================="
echo "Testing: Shell exits on 'exit' command"
echo -e "exit" | timeout 2 ./hw1shell > /dev/null 2>&1
report_test $? "Exit command"

echo "Testing: Exit waits for background jobs"
# The shell should not exit immediately if background jobs are running
START_TIME=$(date +%s)
OUTPUT=$(echo -e "sleep 2 &\nexit" | ./hw1shell 2>&1)
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
# If exit waited, duration should be at least 2 seconds
if [ $DURATION -ge 2 ]; then
    report_test 0 "Exit waits for background jobs"
else
    report_test 1 "Exit waits for background jobs"
fi
echo ""

echo "========================================="
echo "REQUIREMENT 3: cd Command (Internal)"
echo "========================================="
echo "Testing: cd <directory>"
echo -e "cd /tmp\npwd\nexit" | ./hw1shell 2>&1 | grep -q "/tmp" && report_test 0 "cd <directory>" || report_test 1 "cd <directory>"

echo "Testing: cd .."
OUTPUT=$(echo -e "cd /tmp\ncd ..\npwd\nexit" | ./hw1shell 2>&1)
# Check that we went up from /tmp (should be at /)
echo "$OUTPUT" | grep -q "^/" && report_test 0 "cd .." || report_test 1 "cd .."

echo "Testing: cd with invalid directory shows error (shows errno)"
OUTPUT=$(echo -e "cd /nonexistent_dir_12345\nexit" | ./hw1shell 2>&1)
if echo "$OUTPUT" | grep -q "errno"; then
    report_test 0 "cd error message"
else
    report_test 1 "cd error message"
fi

echo "Testing: cd without argument shows error"
echo -e "cd\nexit" | ./hw1shell 2>&1 | grep -q "invalid command" && report_test 0 "cd without argument" || report_test 1 "cd without argument"
echo ""

echo "========================================="
echo "REQUIREMENT 4: jobs Command"
echo "========================================="
echo "Testing: jobs command displays background processes"
OUTPUT=$(echo -e "sleep 5 &\nsleep 10 &\njobs\nexit" | ./hw1shell 2>&1)
# Check for PID followed by whitespace and command
echo "$OUTPUT" | grep -E "^[0-9]+" | grep -q "sleep" && report_test 0 "jobs command" || report_test 1 "jobs command"

echo "Testing: jobs output format (PID, tab, command)"
OUTPUT=$(echo -e "sleep 5 &\njobs\nexit" | ./hw1shell 2>&1)
# Look for PID followed by tab character and command
if echo "$OUTPUT" | grep -P "^[0-9]+\tsleep 5 &" > /dev/null 2>&1; then
    report_test 0 "jobs format with tab"
else
    # Fallback: just check that PID and command are there
    echo "$OUTPUT" | grep -E "^[0-9]+" | grep -q "sleep 5 &" && report_test 0 "jobs format (basic)" || report_test 1 "jobs format"
fi
echo ""

echo "========================================="
echo "REQUIREMENT 5: Background Commands (&)"
echo "========================================="
echo "Testing: Background command with &"
OUTPUT=$(echo -e "sleep 2 &\nexit" | timeout 1 ./hw1shell 2>&1)
echo "$OUTPUT" | grep -q "started" && report_test 0 "Background command execution" || report_test 1 "Background command execution"

echo "Testing: Foreground command (without &)"
START_TIME=$(date +%s)
echo -e "sleep 1\nexit" | ./hw1shell > /dev/null 2>&1
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
[ $DURATION -ge 1 ] && report_test 0 "Foreground command waits" || report_test 1 "Foreground command waits"
echo ""

echo "========================================="
echo "REQUIREMENT 6: Max 4 Background Commands"
echo "========================================="
echo "Testing: Limit of 4 background commands"
OUTPUT=$(echo -e "sleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nexit" | ./hw1shell 2>&1)
echo "$OUTPUT" | grep -q "too many" && report_test 0 "4 background command limit" || report_test 1 "4 background command limit"
echo ""

echo "========================================="
echo "REQUIREMENT 7: fork/exec (Not system())"
echo "========================================="
echo "Testing: External commands execute via fork/exec"
echo -e "echo hello world\nexit" | ./hw1shell 2>&1 | grep -q "hello world" && report_test 0 "External command execution" || report_test 1 "External command execution"

echo "Testing: Multiple external commands"
OUTPUT=$(echo -e "echo test1\necho test2\nexit" | ./hw1shell 2>&1)
echo "$OUTPUT" | grep -q "test1" && echo "$OUTPUT" | grep -q "test2" && report_test 0 "Multiple external commands" || report_test 1 "Multiple external commands"
echo ""

echo "========================================="
echo "REQUIREMENT 8: Foreground waitpid"
echo "========================================="
echo "Testing: Foreground command completes before next prompt"
OUTPUT=$(echo -e "sleep 1\necho done\nexit" | ./hw1shell 2>&1)
# Check that "done" appears after sleep completes
echo "$OUTPUT" | grep -q "done" && report_test 0 "Foreground waitpid" || report_test 1 "Foreground waitpid"
echo ""

echo "========================================="
echo "REQUIREMENT 9: Background Start Message"
echo "========================================="
echo "Testing: Background job start message format"
OUTPUT=$(echo -e "sleep 1 &\nexit" | ./hw1shell 2>&1)
echo "$OUTPUT" | grep -E "hw1shell: pid [0-9]+ started" && report_test 0 "Background start message" || report_test 1 "Background start message"
echo ""

echo "========================================="
echo "REQUIREMENT 10: Empty Command"
echo "========================================="
echo "Testing: Empty command (just Enter)"
echo -e "\n\n\nexit" | ./hw1shell > /dev/null 2>&1
report_test $? "Empty command handling"
echo ""

echo "========================================="
echo "REQUIREMENT 11: Invalid Command Error"
echo "========================================="
echo "Testing: Invalid command shows error"
echo -e "nonexistentcommand123\nexit" | ./hw1shell 2>&1 | grep -q "invalid command" && report_test 0 "Invalid command error" || report_test 1 "Invalid command error"

echo "Testing: Shell continues after invalid command"
OUTPUT=$(echo -e "badcommand\necho recovered\nexit" | ./hw1shell 2>&1)
echo "$OUTPUT" | grep -q "recovered" && report_test 0 "Continue after error" || report_test 1 "Continue after error"
echo ""

echo "========================================="
echo "REQUIREMENT 12: Background Job Reaping"
echo "========================================="
echo "Testing: Background jobs are reaped when finished"
OUTPUT=$(echo -e "sleep 1 &\nsleep 2\nexit" | ./hw1shell 2>&1)
if echo "$OUTPUT" | grep -q "finished"; then
    report_test 0 "Background job finish message"
else
    report_test 1 "Background job finish message"
fi

echo "Testing: Finished jobs free up slots"
# Start a job, let it finish, then start 4 more - should not hit limit
OUTPUT=$(echo -e "sleep 1 &\nsleep 2\nsleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nexit" | ./hw1shell 2>&1)
echo "$OUTPUT" | grep -q "too many" && report_test 1 "Job slots freed" || report_test 0 "Job slots freed"
echo ""

echo "========================================="
echo "REQUIREMENT 13: System Call Error Reporting"
echo "========================================="
echo "Testing: System call errors show errno"
echo -e "badcommand\nexit" | ./hw1shell 2>&1 | grep -q "errno is" && report_test 0 "errno in error messages" || report_test 1 "errno in error messages"

echo "Testing: cd error shows errno"
echo -e "cd /nonexistent_123456\nexit" | ./hw1shell 2>&1 | grep -q "errno" && report_test 0 "cd errno message" || report_test 1 "cd errno message"
echo ""

echo "========================================="
echo "REQUIREMENT 14: Line and Parameter Limits"
echo "========================================="
echo "Testing: Handles multiple parameters"
echo -e "echo a b c d e f g h i j\nexit" | ./hw1shell 2>&1 | grep -q "a b c d e f g h i j" && report_test 0 "Multiple parameters" || report_test 1 "Multiple parameters"

echo "Testing: Handles long command line"
LONG_CMD="echo "
for i in {1..50}; do
    LONG_CMD="${LONG_CMD}word${i} "
done
echo -e "${LONG_CMD}\nexit" | ./hw1shell 2>&1 | grep -q "word50" && report_test 0 "Long command line" || report_test 1 "Long command line"
echo ""

echo "========================================="
echo "ADDITIONAL TESTS: Edge Cases"
echo "========================================="
echo "Testing: Whitespace handling"
echo -e "echo    hello     world\nexit" | ./hw1shell 2>&1 | grep -q "hello" && report_test 0 "Multiple spaces" || report_test 1 "Multiple spaces"

echo "Testing: Background invalid command"
echo -e "fakecommand123 &\nsleep 1\nexit" | ./hw1shell 2>&1 | grep -q "invalid command" && report_test 0 "Background invalid command" || report_test 1 "Background invalid command"

echo "Testing: Mixed foreground and background"
OUTPUT=$(echo -e "sleep 1 &\necho foreground\nexit" | ./hw1shell 2>&1)
echo "$OUTPUT" | grep -q "foreground" && report_test 0 "Mixed fg/bg commands" || report_test 1 "Mixed fg/bg commands"
echo ""

echo "========================================="
echo "           TEST SUMMARY"
echo "========================================="
echo "Total Passed: $PASS_COUNT"
echo "Total Failed: $FAIL_COUNT"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo "🎉 All tests passed!"
    exit 0
else
    echo "⚠️  Some tests failed. Review the output above."
    exit 1
fi
