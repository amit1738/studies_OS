#!/bin/bash
# Error handling tests

echo "=== Error Handling Tests ==="
echo ""

echo "Test 1: Invalid command"
echo -e "notacommand123\nexit" | ./hw1shell 2>&1

echo ""
echo "Test 2: Invalid background command"
echo -e "fakecmd &\nsleep 1\nexit" | ./hw1shell 2>&1

echo ""
echo "Test 3: cd to invalid directory"
echo -e "cd /nonexistent_directory_xyz\nexit" | ./hw1shell 2>&1

echo ""
echo "Test 4: cd without argument"
echo -e "cd\nexit" | ./hw1shell 2>&1

echo ""
echo "Test 5: Check errno in error messages"
echo -e "badcmd\nexit" | ./hw1shell 2>&1 | grep "errno is"

echo ""
echo "Test 6: Multiple errors"
echo -e "error1\ncd /fake\nerror2\nexit" | ./hw1shell 2>&1
