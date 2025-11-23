#!/bin/bash
# Stress and Unpredictable Tests for SmaSh
# These tests cover edge cases, boundary conditions, and unusual inputs

cd "$(dirname "$0")/.."

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

PASSED=0
FAILED=0

run_test() {
    local name="$1"
    local input="$2"
    local expected="$3"
    local check_type="${4:-contains}"

    output=$(echo -e "$input" | timeout 5 ./smash 2>&1)

    case "$check_type" in
        "contains")
            if echo "$output" | grep -q "$expected"; then
                echo -e "${GREEN}[PASS]${NC} $name"
                ((PASSED++))
            else
                echo -e "${RED}[FAIL]${NC} $name"
                echo "  Expected to contain: $expected"
                echo "  Got: $output"
                ((FAILED++))
            fi
            ;;
        "not_contains")
            if ! echo "$output" | grep -q "$expected"; then
                echo -e "${GREEN}[PASS]${NC} $name"
                ((PASSED++))
            else
                echo -e "${RED}[FAIL]${NC} $name"
                echo "  Expected NOT to contain: $expected"
                echo "  Got: $output"
                ((FAILED++))
            fi
            ;;
        "exact_line")
            if echo "$output" | grep -Fxq "$expected"; then
                echo -e "${GREEN}[PASS]${NC} $name"
                ((PASSED++))
            else
                echo -e "${RED}[FAIL]${NC} $name"
                echo "  Expected exact line: $expected"
                echo "  Got: $output"
                ((FAILED++))
            fi
            ;;
    esac
}

echo -e "${BLUE}=== STRESS AND UNPREDICTABLE TESTS ===${NC}"
echo ""

# ============================================
echo -e "${BLUE}=== 1. BUFFER/STRING BOUNDARY TESTS ===${NC}"
# ============================================

# Very long command name
LONG_CMD=$(python3 -c "print('a' * 500)")
run_test "very long command name" "${LONG_CMD}\nquit kill" "command not found"

# Very long argument
run_test "very long argument to echo" "echo $(python3 -c "print('x' * 300)")\nquit kill" "xxx"

# Many arguments
MANY_ARGS=$(python3 -c "print(' '.join(['arg'+str(i) for i in range(50)]))")
run_test "many arguments to echo" "echo ${MANY_ARGS}\nquit kill" "arg0"

# Empty string argument
run_test "echo with empty-like args" "echo    \nquit kill" "basmalamhmdshell"

# ============================================
echo -e "${BLUE}=== 2. SPECIAL CHARACTER TESTS ===${NC}"
# ============================================

# Paths with special chars (create temp files)
mkdir -p /tmp/smash_test_dir
touch "/tmp/smash_test_dir/file1.txt"
touch "/tmp/smash_test_dir/file2.txt"

run_test "cd to path with underscore" "cd /tmp/smash_test_dir\npwd\nquit kill" "/tmp/smash_test_dir"

# Diff with special path names
run_test "diff files in special dir" "diff /tmp/smash_test_dir/file1.txt /tmp/smash_test_dir/file2.txt\nquit kill" "0"

# Command with numbers
run_test "command starting with number" "123command\nquit kill" "command not found"

# ============================================
echo -e "${BLUE}=== 3. RAPID COMMAND SEQUENCES ===${NC}"
# ============================================

# Many pwd commands in sequence
run_test "20 pwd commands" "pwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\npwd\nquit kill" "$(pwd)"

# Rapid cd commands
run_test "rapid cd sequence" "cd /tmp\ncd /var\ncd /tmp\ncd /var\ncd /tmp\npwd\nquit kill" "/tmp"

# Alternating cd - rapidly
run_test "rapid cd - alternation" "cd /tmp\ncd /var\ncd -\ncd -\ncd -\ncd -\ncd -\npwd\nquit kill" "/tmp\|/var"

# ============================================
echo -e "${BLUE}=== 4. JOB MANAGEMENT STRESS ===${NC}"
# ============================================

# Create and kill multiple jobs rapidly
run_test "create 3 background jobs" "sleep 100 &\nsleep 100 &\nsleep 100 &\njobs\nquit kill" "sleep 100"

# Job ID reuse after kill
run_test "job ID 1 reused after termination" "sleep 100 &\nkill 9 1\nsleep 100 &\njobs\nquit kill" "[1]"

# Kill non-existent job multiple times
run_test "kill non-existent job twice" "kill 9 999\nkill 9 999\nquit kill" "does not exist"

# fg on empty list multiple times
run_test "fg on empty list twice" "fg\nfg\nquit kill" "jobs list is empty"

# bg on empty list
run_test "bg with no stopped jobs twice" "bg\nbg\nquit kill" "no stopped jobs"

# ============================================
echo -e "${BLUE}=== 5. WHITESPACE EDGE CASES ===${NC}"
# ============================================

# Only whitespace
run_test "only spaces" "     \nquit kill" "basmalamhmdshell"

# Only tabs
run_test "only tabs" "		\nquit kill" "basmalamhmdshell"

# Mixed whitespace before command
run_test "mixed whitespace before pwd" "   	  pwd\nquit kill" "$(pwd)"

# Whitespace between args
run_test "multiple spaces between args" "echo    hello     world\nquit kill" "hello"

# Trailing whitespace
run_test "trailing spaces after command" "pwd   \nquit kill" "$(pwd)"

# ============================================
echo -e "${BLUE}=== 6. CD EDGE CASES ===${NC}"
# ============================================

# cd to current directory
run_test "cd to current dir (.)" "cd .\npwd\nquit kill" "$(pwd)"

# cd .. at various depths
run_test "cd .. then .. then .." "cd /tmp\ncd /var/log\ncd ..\ncd ..\npwd\nquit kill" "/"

# cd - without any prior cd (should error)
run_test "cd - as first command" "cd -\nquit kill" "old pwd not set"

# cd to same directory twice
run_test "cd to same dir twice" "cd /tmp\ncd /tmp\npwd\nquit kill" "/tmp"

# cd - after cd to same dir
run_test "cd - after cd same dir" "cd /tmp\ncd /tmp\ncd -\npwd\nquit kill" "/tmp"

# ============================================
echo -e "${BLUE}=== 7. DIFF EDGE CASES ===${NC}"
# ============================================

# Create test files
echo "test content" > /tmp/smash_diff1.txt
echo "test content" > /tmp/smash_diff2.txt
echo "different" > /tmp/smash_diff3.txt
echo "" > /tmp/smash_empty.txt

# Diff empty files
touch /tmp/smash_empty1.txt
touch /tmp/smash_empty2.txt
run_test "diff two empty files" "diff /tmp/smash_empty1.txt /tmp/smash_empty2.txt\nquit kill" "0"

# Diff empty vs non-empty
run_test "diff empty vs non-empty" "diff /tmp/smash_empty.txt /tmp/smash_diff1.txt\nquit kill" "1"

# Diff with same file path twice
run_test "diff same file with itself" "diff /tmp/smash_diff1.txt /tmp/smash_diff1.txt\nquit kill" "0"

# Diff with symlink
ln -sf /tmp/smash_diff1.txt /tmp/smash_link.txt 2>/dev/null
run_test "diff file with its symlink" "diff /tmp/smash_diff1.txt /tmp/smash_link.txt\nquit kill" "0"

# ============================================
echo -e "${BLUE}=== 8. KILL SIGNAL EDGE CASES ===${NC}"
# ============================================

# Kill with signal 0 (check if process exists)
run_test "kill with signal 0" "sleep 100 &\nkill 0 1\nquit kill" "signal 0 was sent"

# Kill with various signals
run_test "kill with SIGTERM (15)" "sleep 100 &\nkill 15 1\nquit kill" "signal 15 was sent"

# Kill with SIGKILL (9)
run_test "kill with SIGKILL (9)" "sleep 100 &\nkill 9 1\nquit kill" "signal 9 was sent"

# Kill with invalid job ID 0
run_test "kill job ID 0" "kill 9 0\nquit kill" "does not exist"

# Kill with negative job ID
run_test "kill negative job ID" "kill 9 -1\nquit kill" "does not exist"

# ============================================
echo -e "${BLUE}=== 9. QUIT EDGE CASES ===${NC}"
# ============================================

# quit with random word (not "kill")
run_test "quit with invalid arg" "quit foo\nquit kill" "unexpected arguments"

# quit with multiple args
run_test "quit with 3 args" "quit kill extra\nquit kill" "expected 0 or 1"

# quit kill with no jobs (should just exit)
run_test "quit kill with no jobs" "quit kill" "basmalamhmdshell"

# ============================================
echo -e "${BLUE}=== 10. SHOWPID EDGE CASES ===${NC}"
# ============================================

# showpid returns valid PID
run_test "showpid returns numeric PID" "showpid\nquit kill" "smash pid is [0-9]"

# showpid multiple times same PID (within same shell instance)
output=$(echo -e "showpid\nshowpid\nquit kill" | ./smash 2>&1 | grep "smash pid")
pid1=$(echo "$output" | head -1 | grep -o '[0-9]*')
pid2=$(echo "$output" | tail -1 | grep -o '[0-9]*')
if [ "$pid1" = "$pid2" ] && [ -n "$pid1" ]; then
    echo -e "${GREEN}[PASS]${NC} showpid returns same PID"
    ((PASSED++))
else
    echo -e "${RED}[FAIL]${NC} showpid returns same PID"
    echo "  PID1: $pid1, PID2: $pid2"
    ((FAILED++))
fi

# ============================================
echo -e "${BLUE}=== 11. EXTERNAL COMMAND EDGE CASES ===${NC}"
# ============================================

# Command with absolute path
run_test "absolute path /bin/ls" "/bin/ls /tmp\nquit kill" "smash"

# Command that doesn't exist
run_test "nonexistent command" "thiscommanddoesnotexist\nquit kill" "command not found"

# Command with only numbers in name
run_test "numeric command name" "12345\nquit kill" "command not found"

# Running true and false
run_test "true command" "true\nquit kill" "basmalamhmdshell"
run_test "false command" "false\nquit kill" "basmalamhmdshell"

# ============================================
echo -e "${BLUE}=== 12. BACKGROUND JOB EDGE CASES ===${NC}"
# ============================================

# Background with & attached to command
run_test "background job shows in jobs" "sleep 60 &\njobs\nquit kill" "sleep 60 &"

# & in middle is treated as literal argument, not background operator
run_test "& in middle is literal" "echo hello & world\njobs\nquit kill" "hello & world"

# ============================================
echo -e "${BLUE}=== 13. MIXED OPERATIONS ===${NC}"
# ============================================

# cd then external command
run_test "cd then ls" "cd /tmp\nls\nquit kill" "basmalamhmdshell"

# Multiple built-ins then external
run_test "built-ins then external" "pwd\nshowpid\necho test\nquit kill" "test"

# Create job, cd, check jobs
run_test "job persists after cd" "sleep 60 &\ncd /tmp\njobs\nquit kill" "sleep 60"

# ============================================
echo -e "${BLUE}=== 14. ARGUMENT PARSING EDGE CASES ===${NC}"
# ============================================

# External echo interprets -n as no-newline flag (expected behavior)
run_test "echo -n suppresses newline" "echo -n test\nquit kill" "testbasmalamhmdshell"

# Args with equals sign
run_test "echo with equals" "echo foo=bar\nquit kill" "foo=bar"

# Args with quotes (note: smash doesn't handle quotes specially)
run_test "echo with quote chars" "echo \"hello\"\nquit kill" "\"hello\""

# ============================================
echo -e "${BLUE}=== 15. SEQUENCE STRESS TEST ===${NC}"
# ============================================

# Long sequence of mixed commands
LONG_SEQ="showpid\npwd\ncd /tmp\npwd\ncd -\nshowpid\ncd /var\npwd\ncd ..\npwd\ncd /tmp\nsleep 10 &\njobs\nkill 9 1\njobs\nquit kill"
run_test "long mixed command sequence" "$LONG_SEQ" "basmalamhmdshell"

# Cleanup
rm -f /tmp/smash_diff1.txt /tmp/smash_diff2.txt /tmp/smash_diff3.txt
rm -f /tmp/smash_empty.txt /tmp/smash_empty1.txt /tmp/smash_empty2.txt
rm -f /tmp/smash_link.txt
rm -rf /tmp/smash_test_dir

# ============================================
echo ""
echo "========================================"
echo "   Stress Test Results"
echo "========================================"
echo -e "Passed: ${GREEN}${PASSED}${NC}"
echo -e "Failed: ${RED}${FAILED}${NC}"
echo "Total:  $((PASSED + FAILED))"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All stress tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Review output above.${NC}"
    exit 1
fi
