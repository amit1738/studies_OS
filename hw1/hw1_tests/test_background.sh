#!/bin/bash
# Background job tests

echo "=== Background Job Tests ==="
echo ""

echo "Test 1: Start background job"
echo -e "sleep 2 &\nexit" | ./hw1shell

echo ""
echo "Test 2: Multiple background jobs"
echo -e "sleep 1 &\nsleep 1 &\nsleep 1 &\njobs\nexit" | ./hw1shell

echo ""
echo "Test 3: Background job completes"
echo -e "sleep 1 &\nsleep 2\njobs\nexit" | ./hw1shell

echo ""
echo "Test 4: Fill all 4 slots"
echo -e "sleep 5 &\nsleep 5 &\nsleep 5 &\nsleep 5 &\njobs\nexit" | ./hw1shell

echo ""
echo "Test 5: Try to exceed limit"
echo -e "sleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nexit" | ./hw1shell

echo ""
echo "Test 6: Background with foreground"
echo -e "sleep 2 &\necho middle\nsleep 1\nexit" | ./hw1shell

echo ""
echo "Test 7: Jobs command when empty"
echo -e "jobs\nexit" | ./hw1shell
