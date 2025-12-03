#!/bin/bash
# Edge case tests

echo "=== Edge Case Tests ==="
echo ""

echo "Test 1: Empty command (just Enter)"
echo -e "\nexit" | ./hw1shell

echo ""
echo "Test 2: Multiple empty lines"
echo -e "\n\n\n\nexit" | ./hw1shell

echo ""
echo "Test 3: Whitespace only"
echo -e "   \nexit" | ./hw1shell

echo ""
echo "Test 4: Tabs in input"
printf "echo\thello\nexit\n" | ./hw1shell

echo ""
echo "Test 5: Multiple spaces between args"
echo -e "echo    hello     world\nexit" | ./hw1shell

echo ""
echo "Test 6: & with spaces"
echo -e "sleep 1   &\nsleep 1\nexit" | ./hw1shell

echo ""
echo "Test 7: Command after cd"
echo -e "cd /tmp\npwd\nls\nexit" | ./hw1shell | head -10

echo ""
echo "Test 8: Long argument list"
echo -e "echo 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15\nexit" | ./hw1shell

echo ""
echo "Test 9: Special characters in echo"
echo -e "echo test!@#\$%\nexit" | ./hw1shell

echo ""
echo "Test 10: Reaping with empty input"
echo "Starting background job, then sending empty lines..."
echo -e "sleep 1 &\n\n\njobs\nexit" | ./hw1shell
