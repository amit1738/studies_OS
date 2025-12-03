# hw1shell Test Suite

## Quick Start

### Run All Tests
```bash
chmod +x test_all.sh
./test_all.sh
```

### Run Specific Test Categories

1. **Basic Commands**
```bash
chmod +x test_basic.sh
./test_basic.sh
```
Tests: ls, echo, pwd, multiple commands

2. **Background Jobs**
```bash
chmod +x test_background.sh
./test_background.sh
```
Tests: background execution, jobs command, 4-job limit

3. **Error Handling**
```bash
chmod +x test_errors.sh
./test_errors.sh
```
Tests: invalid commands, errno messages, cd errors

4. **Edge Cases**
```bash
chmod +x test_edge.sh
./test_edge.sh
```
Tests: empty input, whitespace, special characters

5. **Requirements Verification**
```bash
chmod +x test_requirements.sh
./test_requirements.sh
```
Quick check that all 14 requirements are met

## Make All Executable at Once
```bash
chmod +x test_*.sh
```

## Individual Test Commands

### Test basic functionality:
```bash
echo -e "ls\necho hello\nexit" | ./hw1shell
```

### Test background jobs:
```bash
echo -e "sleep 2 &\njobs\nexit" | ./hw1shell
```

### Test cd command:
```bash
echo -e "cd /tmp\npwd\ncd ..\npwd\nexit" | ./hw1shell
```

### Test error handling:
```bash
echo -e "invalidcommand\nexit" | ./hw1shell
```

### Test 4-job limit:
```bash
echo -e "sleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nsleep 10 &\nexit" | ./hw1shell
```

## Expected Behavior

### Successful Tests Show:
- ✅ PASS - Test passed
- Commands execute correctly
- Background jobs show "pid X started" and "pid X finished"
- Errors show "errno is X"

### Common Issues:
- If tests hang, press Ctrl+C
- If zombie processes appear, they should be cleaned within 1-2 seconds
- Background jobs should complete and be reaped automatically

## Compilation
```bash
make clean
make
```

## Manual Testing
To test interactively:
```bash
./hw1shell
hw1shell$ ls
hw1shell$ echo test
hw1shell$ sleep 2 &
hw1shell$ jobs
hw1shell$ exit
```
