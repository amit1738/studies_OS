#!/bin/bash
# Basic command tests

echo "=== Basic Command Tests ==="
echo ""

echo "Test 1: Simple ls"
echo -e "ls\nexit" | ./hw1shell | head -3

echo ""
echo "Test 2: Echo with multiple arguments"
echo -e "echo hello world test\nexit" | ./hw1shell

echo ""
echo "Test 3: pwd command"
echo -e "pwd\nexit" | ./hw1shell

echo ""
echo "Test 4: Command with path"
echo -e "/bin/echo testing\nexit" | ./hw1shell

echo ""
echo "Test 5: Multiple commands in sequence"
echo -e "echo first\necho second\necho third\nexit" | ./hw1shell
