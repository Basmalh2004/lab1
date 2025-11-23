#!/bin/bash

# Test Suite for SmaSh (Small Shell)
# ===================================

SMASH="/home/user/lab1/smash"
PASSED=0
FAILED=0
TOTAL=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test function
run_test() {
    local test_name="$1"
    local input="$2"
    local expected_pattern="$3"
    local should_match="${4:-1}"  # 1 = should match, 0 = should NOT match

    TOTAL=$((TOTAL + 1))

    # Run the command and capture output
    output=$(echo -e "$input" | timeout 2s $SMASH 2>&1)

    if [ "$should_match" = "1" ]; then
        if echo "$output" | grep -qE "$expected_pattern"; then
            echo -e "${GREEN}[PASS]${NC} $test_name"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}[FAIL]${NC} $test_name"
            echo "       Expected pattern: $expected_pattern"
            echo "       Got: $output" | head -3
            FAILED=$((FAILED + 1))
        fi
    else
        if echo "$output" | grep -qE "$expected_pattern"; then
            echo -e "${RED}[FAIL]${NC} $test_name"
            echo "       Should NOT match: $expected_pattern"
            FAILED=$((FAILED + 1))
        else
            echo -e "${GREEN}[PASS]${NC} $test_name"
            PASSED=$((PASSED + 1))
        fi
    fi
}

# Test with exact output match
run_test_exact() {
    local test_name="$1"
    local input="$2"
    local expected="$3"

    TOTAL=$((TOTAL + 1))

    output=$(echo -e "$input" | timeout 2s $SMASH 2>&1)

    if echo "$output" | grep -qF "$expected"; then
        echo -e "${GREEN}[PASS]${NC} $test_name"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}[FAIL]${NC} $test_name"
        echo "       Expected: $expected"
        echo "       Got: $output" | head -3
        FAILED=$((FAILED + 1))
    fi
}

echo "========================================"
echo "   SmaSh Test Suite"
echo "========================================"
echo ""

# Create test directory
mkdir -p test_data
cd test_data

# Create test files for diff command
echo "hello world" > file1.txt
echo "hello world" > file2.txt
echo "different content" > file3.txt
mkdir testdir

echo -e "${YELLOW}=== Testing Command Parsing ===${NC}"

run_test "Empty command handling" "\n" "basmalamhmdshell"

echo ""
echo -e "${YELLOW}=== Testing showpid ===${NC}"

run_test "showpid returns PID" "showpid\nquit kill" "smash pid is [0-9]+"
run_test "showpid with extra args fails" "showpid extra\nquit kill" "expected 0 arguments"

echo ""
echo -e "${YELLOW}=== Testing pwd ===${NC}"

run_test "pwd returns current directory" "pwd\nquit kill" "/"
run_test "pwd with extra args fails" "pwd extra\nquit kill" "expected 0 arguments"

echo ""
echo -e "${YELLOW}=== Testing cd ===${NC}"

run_test "cd to valid directory" "cd /tmp\npwd\nquit kill" "/tmp"
run_test "cd to nonexistent directory fails" "cd /nonexistent_dir_xyz\nquit kill" "does not exist"
run_test "cd to file fails" "cd file1.txt\nquit kill" "not a directory"
run_test "cd - without previous dir fails" "cd -\nquit kill" "old pwd not set"
run_test "cd - returns to previous dir" "cd /tmp\ncd /\ncd -\npwd\nquit kill" "/tmp"
run_test "cd with no args fails" "cd\nquit kill" "expected 1 arguments"
run_test "cd with too many args fails" "cd /tmp extra\nquit kill" "expected 1 arguments"

echo ""
echo -e "${YELLOW}=== Testing diff ===${NC}"

run_test "diff identical files returns 0" "diff file1.txt file2.txt\nquit kill" "> 0"
run_test "diff different files returns 1" "diff file1.txt file3.txt\nquit kill" "> 1"
run_test "diff nonexistent file fails" "diff file1.txt nonexistent.txt\nquit kill" "expected valid paths"
run_test "diff with directory fails" "diff file1.txt testdir\nquit kill" "paths are not files"
run_test "diff with wrong arg count fails" "diff file1.txt\nquit kill" "expected 2 arguments"

echo ""
echo -e "${YELLOW}=== Testing jobs (empty) ===${NC}"

run_test "jobs with empty list" "jobs\nquit kill" "basmalamhmdshell"

echo ""
echo -e "${YELLOW}=== Testing kill ===${NC}"

run_test "kill invalid job fails" "kill 9 999\nquit kill" "job id 999 does not exist"
run_test "kill wrong arg count fails" "kill 9\nquit kill" "invalid arguments"

echo ""
echo -e "${YELLOW}=== Testing fg ===${NC}"

run_test "fg with empty jobs list fails" "fg\nquit kill" "jobs list is empty"
run_test "fg invalid job fails" "fg 999\nquit kill" "job id 999 does not exist"

echo ""
echo -e "${YELLOW}=== Testing bg ===${NC}"

run_test "bg with no stopped jobs fails" "bg\nquit kill" "no stopped jobs"
run_test "bg invalid job fails" "bg 999\nquit kill" "job id 999 does not exist"

echo ""
echo -e "${YELLOW}=== Testing quit ===${NC}"

run_test "quit kill exits cleanly" "quit kill" ""

echo ""
echo -e "${YELLOW}=== Testing External Commands ===${NC}"

run_test "ls command works" "ls\nquit kill" "file1.txt"
run_test "echo command works" "echo hello\nquit kill" "hello"
run_test "invalid command fails" "nonexistent_command_xyz\nquit kill" "command not found"

echo ""
echo -e "${YELLOW}=== Testing Background Jobs ===${NC}"

run_test "background job syntax" "sleep 1 &\njobs\nquit kill" "sleep 1 &"

# Cleanup
cd ..
rm -rf test_data

echo ""
echo "========================================"
echo "   Test Results"
echo "========================================"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo "Total:  $TOTAL"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi
