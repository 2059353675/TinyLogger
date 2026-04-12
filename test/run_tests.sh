#!/bin/bash
# ========================================
# TinyLogger Test Runner (Linux/macOS)
# ========================================
# 
# 此脚本是主构建系统的轻量包装，不会独立构建
# 请确保已执行过：mkdir build && cd build && cmake .. && make
#

echo "========================================"
echo "  TinyLogger Test Suite"
echo "========================================"
echo ""

# 检测构建目录
BUILD_DIR="../build"
if [ ! -d "$BUILD_DIR" ]; then
    echo "错误：未找到构建目录 $BUILD_DIR"
    echo "请先执行以下命令构建测试："
    echo "  cd .."
    echo "  mkdir build && cd build"
    echo "  cmake .."
    echo "  make"
    exit 1
fi

# 检测测试可执行文件是否存在
TEST_BINARIES=("test_ring_buffer" "test_config" "test_printer" "test_distributor" "test_logger")
MISSING=0

for test_bin in "${TEST_BINARIES[@]}"; do
    if [ ! -f "$BUILD_DIR/test/$test_bin" ]; then
        echo "错误：未找到测试可执行文件 $BUILD_DIR/test/$test_bin"
        MISSING=1
    fi
done

if [ $MISSING -eq 1 ]; then
    echo ""
    echo "请先构建测试："
    echo "  cd ../build"
    echo "  make"
    exit 1
fi

# 运行测试
cd "$BUILD_DIR/test"
FAILED=0
TOTAL=0

echo "[1/5] Running RingBuffer Tests..."
./test_ring_buffer
if [ $? -ne 0 ]; then
    FAILED=$((FAILED + 1))
fi
TOTAL=$((TOTAL + 1))
echo ""

echo "[2/5] Running Config Tests..."
./test_config
if [ $? -ne 0 ]; then
    FAILED=$((FAILED + 1))
fi
TOTAL=$((TOTAL + 1))
echo ""

echo "[3/5] Running Printer Tests..."
./test_printer
if [ $? -ne 0 ]; then
    FAILED=$((FAILED + 1))
fi
TOTAL=$((TOTAL + 1))
echo ""

echo "[4/5] Running Distributor Tests..."
./test_distributor
if [ $? -ne 0 ]; then
    FAILED=$((FAILED + 1))
fi
TOTAL=$((TOTAL + 1))
echo ""

echo "[5/5] Running Logger Integration Tests..."
./test_logger
if [ $? -ne 0 ]; then
    FAILED=$((FAILED + 1))
fi
TOTAL=$((TOTAL + 1))
echo ""

cd - > /dev/null

echo "========================================"
if [ $FAILED -eq 0 ]; then
    echo "  All Tests Passed! ($TOTAL/$TOTAL)"
else
    echo "  $FAILED/$TOTAL Test(s) Failed"
fi
echo "========================================"

exit $FAILED
