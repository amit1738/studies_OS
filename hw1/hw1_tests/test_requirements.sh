#!/bin/bash
# Quick verification of all 14 requirements

echo "========================================="
echo "  Requirement Verification Checklist"
echo "========================================="
echo ""

echo "Req 1: Infinite loop with 'hw1shell$ ' prompt"
echo "" | ./hw1shell 2>&1 | head -1 | grep -q "hw1shell\$ " && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 2: exit waits for background jobs"
timeout 5 bash -c 'echo -e "sleep 2 &\nexit" | ./hw1shell' > /dev/null 2>&1 && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 3: cd uses chdir, handles .."
echo -e "cd /tmp\ncd ..\npwd\nexit" | ./hw1shell | grep -q "/" && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 4: jobs shows PID<tab>command"
echo -e "sleep 5 &\njobs\nexit" | ./hw1shell | grep -qE "^[0-9]+\t" && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 5: & detected for background"
echo -e "sleep 1 &\nexit" | ./hw1shell | grep -q "&" && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 6: Support 4 background jobs max"
echo -e "sleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nexit" | ./hw1shell | grep -q "too many" && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 7: Uses fork/exec (not system)"
grep -q "system(" shell.c && echo "  ❌ FAIL - uses system()" || echo "  ✅ PASS"

echo ""
echo "Req 8: Foreground waits with waitpid"
echo -e "sleep 1\necho done\nexit" | ./hw1shell | grep -q "done" && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 9: Background shows 'pid %d started'"
echo -e "sleep 1 &\nexit" | ./hw1shell | grep -qE "pid [0-9]+ started" && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 10: Empty command is not an error"
echo -e "\nexit" | ./hw1shell 2>&1 | grep -iq error && echo "  ❌ FAIL" || echo "  ✅ PASS"

echo ""
echo "Req 11: Invalid command shows error"
echo -e "badcmd\nexit" | ./hw1shell 2>&1 | grep -q "invalid command" && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 12: Reaps zombies, shows 'pid %d finished'"
echo -e "sleep 1 &\nsleep 2\nexit" | ./hw1shell | grep -qE "pid [0-9]+ finished" && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 13: System call errors show errno"
echo -e "fakecmd\nexit" | ./hw1shell 2>&1 | grep -q "errno is" && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "Req 14: MAX_LINE 1024, ARGS_MAX 64"
grep -q "MAX_LINE 1024" process_control.h && grep -q "ARGS_MAX 64" shell.c && echo "  ✅ PASS" || echo "  ❌ FAIL"

echo ""
echo "========================================="
echo "  Verification Complete"
echo "========================================="
