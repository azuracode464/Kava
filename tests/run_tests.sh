#!/bin/bash
# KAVA 2.5 - Test Suite Runner
# Compiles and runs all test files, validates output

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
KAVAC="$ROOT_DIR/kavac"
KAVAVM="$ROOT_DIR/kavavm"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

PASSED=0
FAILED=0
TOTAL=0

run_test() {
    local name="$1"
    local file="$2"
    local expected="$3"
    
    TOTAL=$((TOTAL + 1))
    
    # Compile
    if ! $KAVAC "$file" > /dev/null 2>&1; then
        echo -e "  ${RED}FAIL${NC} $name - compilation failed"
        FAILED=$((FAILED + 1))
        return
    fi
    
    # Get .kvb path
    local kvb="${file%.kava}.kvb"
    
    # Execute
    local actual
    actual=$($KAVAVM "$kvb" 2>&1)
    
    if [ "$actual" = "$expected" ]; then
        echo -e "  ${GREEN}PASS${NC} $name"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}FAIL${NC} $name"
        echo "    Expected: $(echo "$expected" | head -3)..."
        echo "    Got:      $(echo "$actual" | head -3)..."
        FAILED=$((FAILED + 1))
    fi
    
    # Cleanup
    rm -f "$kvb"
}

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║          KAVA 2.5 - Test Suite Runner               ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

# =============================================
# TEST 1: Basic Arithmetic + Variables
# =============================================
echo -e "${CYAN}[Section 1] Basic Arithmetic & Variables${NC}"

cat > /tmp/kava_test_arith.kava << 'EOF'
let a = 10
let b = 20
let soma = a + b
print soma
let mult = a * b
print mult
let div = b / a
print div
let mod = 27 % 10
print mod
let neg = 0 - 5
print neg
EOF
run_test "Arithmetic operations" "/tmp/kava_test_arith.kava" "30
200
2
7
-5"

# =============================================
# TEST 2: If/Else
# =============================================
echo -e "${CYAN}[Section 2] Control Flow: If/Else${NC}"

cat > /tmp/kava_test_if.kava << 'EOF'
let x = 42
if (x == 42) {
    print 1
} else {
    print 0
}
if (x > 50) {
    print 0
} else {
    if (x > 40) {
        print 42
    } else {
        print 0
    }
}
if (x < 100) {
    print 100
}
EOF
run_test "If/Else statements" "/tmp/kava_test_if.kava" "1
42
100"

# =============================================
# TEST 3: While Loop
# =============================================
echo -e "${CYAN}[Section 3] Control Flow: Loops${NC}"

cat > /tmp/kava_test_while.kava << 'EOF'
let i = 0
while (i < 5) {
    print i
    i = i + 1
}
print 99
EOF
run_test "While loop" "/tmp/kava_test_while.kava" "0
1
2
3
4
99"

# =============================================
# TEST 4: Nested Loops
# =============================================
cat > /tmp/kava_test_nested.kava << 'EOF'
let sum = 0
let i = 0
while (i < 10) {
    let j = 0
    while (j < 10) {
        sum = sum + 1
        j = j + 1
    }
    i = i + 1
}
print sum
EOF
run_test "Nested loops" "/tmp/kava_test_nested.kava" "100"

# =============================================
# TEST 5: Bitwise Operations
# =============================================
echo -e "${CYAN}[Section 4] Bitwise Operations${NC}"

cat > /tmp/kava_test_bitwise.kava << 'EOF'
let a = 12
let b = 10
print a & b
print a | b
print a ^ b
print 1 << 4
print 16 >> 2
EOF
run_test "Bitwise operations" "/tmp/kava_test_bitwise.kava" "8
14
6
16
4"

# =============================================
# TEST 6: Compound Assignment
# =============================================
echo -e "${CYAN}[Section 5] Compound Assignment${NC}"

cat > /tmp/kava_test_compound.kava << 'EOF'
let x = 10
x = x + 5
print x
x = x - 3
print x
x = x * 2
print x
EOF
run_test "Compound assignment" "/tmp/kava_test_compound.kava" "15
12
24"

# =============================================
# TEST 7: Boolean Logic
# =============================================
echo -e "${CYAN}[Section 6] Boolean Logic${NC}"

cat > /tmp/kava_test_bool.kava << 'EOF'
let t = 1
let f = 0
if (t == 1) {
    if (f == 0) {
        print 999
    }
}
let x = 5
if (x > 0) {
    if (x < 10) {
        print 1
    }
}
EOF
run_test "Boolean logic" "/tmp/kava_test_bool.kava" "999
1"

# =============================================
# TEST 8: Heavy Computation
# =============================================
echo -e "${CYAN}[Section 7] Heavy Computation${NC}"

cat > /tmp/kava_test_heavy.kava << 'EOF'
let sum = 0
let n = 0
while (n < 1000) {
    sum = sum + n
    n = n + 1
}
print sum
EOF
run_test "Sum 0..999" "/tmp/kava_test_heavy.kava" "499500"

cat > /tmp/kava_test_heavy2.kava << 'EOF'
let sum = 0
let n = 0
while (n < 10000) {
    sum = sum + n
    n = n + 1
}
print sum
EOF
run_test "Sum 0..9999" "/tmp/kava_test_heavy2.kava" "49995000"

# =============================================
# TEST 9: Comparison operators
# =============================================
echo -e "${CYAN}[Section 8] Comparison Operators${NC}"

cat > /tmp/kava_test_cmp.kava << 'EOF'
let a = 10
let b = 20
if (a < b) { print 1 } else { print 0 }
if (a > b) { print 1 } else { print 0 }
if (a <= 10) { print 1 } else { print 0 }
if (b >= 20) { print 1 } else { print 0 }
if (a == 10) { print 1 } else { print 0 }
if (a != b) { print 1 } else { print 0 }
EOF
run_test "Comparison operators" "/tmp/kava_test_cmp.kava" "1
0
1
1
1
1"

# =============================================
# TEST 10: Full KAVA 2.5 Test
# =============================================
echo -e "${CYAN}[Section 9] Full Integration Test${NC}"
run_test "KAVA 2.5 full test" "$ROOT_DIR/examples/test_2_5.kava" "30
200
2
7
1
3
10
42
8
14
6
16
15
12
24
999
100
499500
7777"

run_test "KAVA 2.0 compatibility" "$ROOT_DIR/examples/test_2_0.kava" "30
100
0
1
2
3
4
50"

# =============================================
# SUMMARY
# =============================================
echo ""
echo "════════════════════════════════════════════"
echo "  Results: $PASSED/$TOTAL passed"
if [ $FAILED -eq 0 ]; then
    echo -e "  Status: ${GREEN}ALL TESTS PASSED${NC}"
else
    echo -e "  Status: ${RED}$FAILED TESTS FAILED${NC}"
fi
echo "════════════════════════════════════════════"
echo ""

# Cleanup
rm -f /tmp/kava_test_*.kava /tmp/kava_test_*.kvb

exit $FAILED
