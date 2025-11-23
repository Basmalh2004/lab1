#!/bin/bash

# Edge Case Tests for SmaSh
# =========================

SMASH="/home/user/lab1/smash"
PASSED=0
FAILED=0
TOTAL=0

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

run_test() {
    local test_name="$1"
    local input="$2"
    local expected_pattern="$3"

    TOTAL=$((TOTAL + 1))
    output=$(echo -e "$input" | timeout 3s $SMASH 2>&1)

    if echo "$output" | grep -qE "$expected_pattern"; then
        echo -e "${GREEN}[PASS]${NC} $test_name"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}[FAIL]${NC} $test_name"
        echo "       Expected: $expected_pattern"
        echo "       Got: $(echo "$output" | head -3)"
        FAILED=$((FAILED + 1))
    fi
}

echo "========================================"
echo "   Edge Case Tests"
echo "========================================"
echo ""

# Setup
mkdir -p /tmp/smash_test
cd /tmp/smash_test

echo -e "${YELLOW}=== Parser Edge Cases ===${NC}"

run_test "Multiple spaces between args" "echo    hello     world\nquit kill" "hello"
run_test "Tab as delimiter" "echo\thello\nquit kill" "hello"
run_test "Command with trailing spaces" "pwd   \nquit kill" "/"
run_test "Very long command" "echo aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\nquit kill" "aaa"

echo ""
echo -e "${YELLOW}=== cd Edge Cases ===${NC}"

run_test "cd to root" "cd /\npwd\nquit kill" "> /$"
run_test "cd with .." "cd /tmp\ncd ..\npwd\nquit kill" "/"
run_test "cd consecutive" "cd /tmp\ncd /var\ncd /usr\npwd\nquit kill" "/usr"
run_test "cd - after multiple cds" "cd /tmp\ncd /var\ncd -\npwd\nquit kill" "/tmp"

echo ""
echo -e "${YELLOW}=== diff Edge Cases ===${NC}"

# Create test files
echo "" > empty1.txt
echo "" > empty2.txt
echo "line1" > oneline.txt
dd if=/dev/zero bs=1024 count=10 2>/dev/null | tr '\0' 'A' > large1.txt
dd if=/dev/zero bs=1024 count=10 2>/dev/null | tr '\0' 'A' > large2.txt
dd if=/dev/zero bs=1024 count=10 2>/dev/null | tr '\0' 'B' > large3.txt

run_test "diff empty files" "diff empty1.txt empty2.txt\nquit kill" "> 0"
run_test "diff same file twice" "diff oneline.txt oneline.txt\nquit kill" "> 0"
run_test "diff large identical files" "diff large1.txt large2.txt\nquit kill" "> 0"
run_test "diff large different files" "diff large1.txt large3.txt\nquit kill" "> 1"

echo ""
echo -e "${YELLOW}=== Error Message Tests ===${NC}"

run_test "kill with negative signal" "kill -9 1\nquit kill" "job id"
run_test "kill with string job id" "kill 9 abc\nquit kill" "job id 0 does not exist"
run_test "fg with string job id" "fg abc\nquit kill" "job id 0 does not exist"

echo ""
echo -e "${YELLOW}=== External Command Edge Cases ===${NC}"

run_test "Command with path" "/bin/echo test\nquit kill" "test"
run_test "Command with multiple args" "echo one two three four\nquit kill" "one two three four"

echo ""
echo -e "${YELLOW}=== Background Job Tests ===${NC}"

run_test "Multiple background jobs" "sleep 10 &\nsleep 10 &\njobs\nquit kill" "sleep 10"
run_test "Background job listing format" "sleep 5 &\njobs\nquit kill" "\[1\].*sleep 5 &.*[0-9]+ secs"

# Cleanup
cd /
rm -rf /tmp/smash_test

echo ""
echo "========================================"
echo "   Edge Case Results"
echo "========================================"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo "Total:  $TOTAL"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All edge case tests passed!${NC}"
    exit 0
else
    echo -e "${YELLOW}Some edge cases failed - review needed${NC}"
    exit 1
fi
