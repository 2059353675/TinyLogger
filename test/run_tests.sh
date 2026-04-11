#!/bin/bash
# ========================================
# TinyLogger Test Runner (Linux/macOS)
# ========================================

echo "========================================"
echo "  TinyLogger Test Suite"
echo "========================================"
echo ""

# 设置构建目录
BUILD_DIR="build_test"

# 创建并进入构建目录
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# 配置 CMake
echo "[1/3] Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "FAILED: CMake configuration"
    cd ..
    exit 1
fi
echo ""

# 构建测试
echo "[2/3] Building tests..."
cmake --build . -j$(nproc)
if [ $? -ne 0 ]; then
    echo "FAILED: Build"
    cd ..
    exit 1
fi
echo ""

# 运行测试
echo "[3/3] Running tests..."
echo ""

FAILED=0

echo "Running RingBuffer Tests..."
./test_ring_buffer
if [ $? -ne 0 ]; then
    FAILED=$((FAILED + 1))
fi
echo ""

echo "Running Config Tests..."
./test_config
if [ $? -ne 0 ]; then
    FAILED=$((FAILED + 1))
fi
echo ""

echo "Running Printer Tests..."
./test_printer
if [ $? -ne 0 ]; then
    FAILED=$((FAILED + 1))
fi
echo ""

echo "Running Distributor Tests..."
./test_distributor
if [ $? -ne 0 ]; then
    FAILED=$((FAILED + 1))
fi
echo ""

echo "Running Logger Integration Tests..."
./test_logger
if [ $? -ne 0 ]; then
    FAILED=$((FAILED + 1))
fi
echo ""

cd ..

echo "========================================"
if [ $FAILED -eq 0 ]; then
    echo "  All Tests Passed!"
else
    echo "  $FAILED Test(s) Failed"
fi
echo "========================================"

exit $FAILED
