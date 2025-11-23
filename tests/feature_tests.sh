#!/bin/bash
# Tests for new features: compound commands (&&), alias, unalias

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
    esac
}

echo -e "${BLUE}=== FEATURE TESTS ===${NC}"
echo ""

# ============================================
echo -e "${BLUE}=== 1. COMPOUND COMMANDS (&&) ===${NC}"
# ============================================

# Basic && - both succeed
run_test "&& both succeed" "echo first && echo second\nquit kill" "second"

# && first fails - second not executed
run_test "&& first fails" "/bin/false && echo shouldnot\nquit kill" "shouldnot" "not_contains"

# && chain of 3
run_test "&& chain of 3" "echo 1 && echo 2 && echo 3\nquit kill" "3"

# && chain - middle fails
run_test "&& chain middle fails" "echo 1 && /bin/false && echo 3\nquit kill" "3" "not_contains"

# && with built-in commands
run_test "&& with pwd" "cd /tmp && pwd\nquit kill" "/tmp"

# && with cd failure
run_test "&& with cd failure" "cd /nonexistent && pwd\nquit kill" "target directory does not exist"

# ============================================
echo -e "${BLUE}=== 2. ALIAS COMMAND ===${NC}"
# ============================================

# Create simple alias
run_test "create simple alias" "alias p=\"pwd\"\nalias\nquit kill" "p='pwd'"

# Create alias with args
run_test "create alias with args" "alias ll=\"ls -la\"\nalias\nquit kill" "ll='ls -la'"

# Use alias
output=$(echo -e "alias p=\"pwd\"\np\nquit kill" | ./smash 2>&1)
if echo "$output" | grep -q "$(pwd)"; then
    echo -e "${GREEN}[PASS]${NC} use alias"
    ((PASSED++))
else
    echo -e "${RED}[FAIL]${NC} use alias"
    ((FAILED++))
fi

# Multiple aliases
run_test "multiple aliases" "alias a=\"echo a\"\nalias b=\"echo b\"\nalias\nquit kill" "b='echo b'"

# Update existing alias
run_test "update alias" "alias x=\"old\"\nalias x=\"new\"\nalias\nquit kill" "x='new'"

# Empty alias list
run_test "empty alias list" "alias\nquit kill" "basmalamhmdshell"

# ============================================
echo -e "${BLUE}=== 3. UNALIAS COMMAND ===${NC}"
# ============================================

# Unalias existing
output=$(echo -e "alias x=\"test\"\nunalias x\nalias\nquit kill" | ./smash 2>&1)
if ! echo "$output" | grep -q "x="; then
    echo -e "${GREEN}[PASS]${NC} unalias removes alias"
    ((PASSED++))
else
    echo -e "${RED}[FAIL]${NC} unalias removes alias"
    ((FAILED++))
fi

# Unalias non-existent
run_test "unalias non-existent" "unalias nonexistent\nquit kill" "alias not found"

# Unalias missing arg
run_test "unalias missing arg" "unalias\nquit kill" "expected 1 argument"

# ============================================
echo -e "${BLUE}=== 4. ALIAS WITH && ===${NC}"
# ============================================

# Alias in compound command
run_test "alias in && chain" "alias e=\"echo\"\ne hello && e world\nquit kill" "world"

# Alias that contains &&
run_test "alias containing &&" "alias both=\"echo first && echo second\"\nboth\nquit kill" "second"

# ============================================
echo ""
echo "========================================"
echo "   Feature Test Results"
echo "========================================"
echo -e "Passed: ${GREEN}${PASSED}${NC}"
echo -e "Failed: ${RED}${FAILED}${NC}"
echo "Total:  $((PASSED + FAILED))"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All feature tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Review output above.${NC}"
    exit 1
fi
