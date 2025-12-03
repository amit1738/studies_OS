#!/bin/bash

echo "========================================="
echo "    hw1shell Comprehensive Test Suite"
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

echo "========================================="
echo "TEST 1: Basic Commands"
echo "========================================="
echo "Test: ls command"
echo -e "ls\nexit" | ./hw1shell | head -5
echo ""

echo "Test: echo with arguments"
echo -e "echo hello world\nexit" | ./hw1shell | grep "hello world" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: Empty command"
echo -e "\n\nexit" | ./hw1shell > /dev/null 2>&1 && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "========================================="
echo "TEST 2: Built-in Commands"
echo "========================================="
echo "Test: cd command"
echo -e "cd /tmp\npwd\nexit" | ./hw1shell | grep "/tmp" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: cd .."
echo -e "cd /tmp\ncd ..\npwd\nexit" | ./hw1shell | grep "^/" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: cd with invalid directory"
echo -e "cd /nonexistent123\nexit" | ./hw1shell 2>&1 | grep "errno" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: cd without argument"
echo -e "cd\nexit" | ./hw1shell 2>&1 | grep "invalid command" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "========================================="
echo "TEST 3: Background Jobs"
echo "========================================="
echo "Test: Background job start message"
echo -e "sleep 1 &\nexit" | ./hw1shell | grep "pid .* started" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: Background job finish message"
echo -e "sleep 1 &\nsleep 2\nexit" | ./hw1shell | grep "pid .* finished" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: jobs command"
echo -e "sleep 5 &\nsleep 10 &\njobs\nexit" | ./hw1shell | grep -E "^[0-9]+\t" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: Maximum 4 background jobs"
echo -e "sleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nexit" | ./hw1shell | grep "too many" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: Background job reaping with empty input"
echo -e "sleep 1 &\n\n\njobs\nexit" | ./hw1shell > /tmp/test_reap.txt 2>&1
sleep 2
# Check that job doesn't appear in jobs list (it should be reaped)
cat /tmp/test_reap.txt | tail -3 | grep -E "^[0-9]+\t" && echo "❌ FAIL - job not reaped" || echo "✅ PASS - job reaped"
rm -f /tmp/test_reap.txt
echo ""

echo "========================================="
echo "TEST 4: Error Handling"
echo "========================================="
echo "Test: Invalid command"
echo -e "nonexistentcmd\nexit" | ./hw1shell 2>&1 | grep "invalid command" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: System call error shows errno"
echo -e "badcommand\nexit" | ./hw1shell 2>&1 | grep "errno is" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: Background invalid command"
echo -e "fakecmd &\nsleep 1\nexit" | ./hw1shell 2>&1 | grep "invalid command" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "========================================="
echo "TEST 5: Edge Cases"
echo "========================================="
echo "Test: Multiple spaces"
echo -e "echo    hello     world\nexit" | ./hw1shell | grep "hello" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: Whitespace only"
echo -e "   \t   \nexit" | ./hw1shell > /dev/null 2>&1 && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: Foreground then background"
echo -e "echo test\nsleep 1 &\necho done\nexit" | ./hw1shell | grep -c "done" | grep -q "1" && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "Test: Exit waits for background jobs"
timeout 5 bash -c 'echo -e "sleep 2 &\nexit" | ./hw1shell' > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✅ PASS - exit waited for background"
else
    echo "❌ FAIL - exit didn't wait or timeout"
fi
echo ""

echo "========================================="
echo "TEST 6: Prompt Format"
echo "========================================="
echo "Test: Prompt is 'hw1shell$ '"
echo "" | ./hw1shell 2>&1 | head -1 | grep "hw1shell\$ " && echo "✅ PASS" || echo "❌ FAIL"
echo ""

echo "========================================="
echo "    Test Suite Complete"
echo "========================================="
echo ""
echo "To run individual tests:"
echo "  ./test_basic.sh      - Basic command tests"
echo "  ./test_background.sh - Background job tests"
echo "  ./test_errors.sh     - Error handling tests"
echo "  ./test_edge.sh       - Edge case tests"
