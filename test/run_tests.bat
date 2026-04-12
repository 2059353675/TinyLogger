@echo off
REM ========================================
REM  TinyLogger Test Runner (Windows)
REM ========================================
REM
REM  此脚本是主构建系统的轻量包装，不会独立构建
REM  请确保已执行过：mkdir build ^&^& cd build ^&^& cmake .. ^&^& cmake --build .
REM

echo ========================================
echo   TinyLogger Test Suite
echo ========================================
echo.

REM 检测构建目录
set BUILD_DIR=..\build
if not exist "%BUILD_DIR%" (
    echo 错误：未找到构建目录 %BUILD_DIR%
    echo 请先执行以下命令构建测试：
    echo   cd ..
    echo   mkdir build ^&^& cd build
    echo   cmake ..
    echo   cmake --build .
    exit /b 1
)

REM 检测测试可执行文件是否存在
set TEST_BINARIES=test_ring_buffer.exe test_config.exe test_printer.exe test_distributor.exe test_logger.exe
set MISSING=0

for %%t in (%TEST_BINARIES%) do (
    if not exist "%BUILD_DIR%\test\%%t" (
        echo 错误：未找到测试可执行文件 %BUILD_DIR%\test\%%t
        set MISSING=1
    )
)

if %MISSING%==1 (
    echo.
    echo 请先构建测试：
    echo   cd ..\build
    echo   cmake --build .
    exit /b 1
)

REM 运行测试
cd "%BUILD_DIR%\test"
set FAILED=0
set TOTAL=0

echo [1/5] Running RingBuffer Tests...
call test_ring_buffer.exe
if errorlevel 1 set /a FAILED+=1
set /a TOTAL+=1
echo.

echo [2/5] Running Config Tests...
call test_config.exe
if errorlevel 1 set /a FAILED+=1
set /a TOTAL+=1
echo.

echo [3/5] Running Printer Tests...
call test_printer.exe
if errorlevel 1 set /a FAILED+=1
set /a TOTAL+=1
echo.

echo [4/5] Running Distributor Tests...
call test_distributor.exe
if errorlevel 1 set /a FAILED+=1
set /a TOTAL+=1
echo.

echo [5/5] Running Logger Integration Tests...
call test_logger.exe
if errorlevel 1 set /a FAILED+=1
set /a TOTAL+=1
echo.

cd /d "%~dp0"

echo ========================================
if %FAILED%==0 (
    echo   All Tests Passed! (!TOTAL!/!TOTAL!)
) else (
    echo   %FAILED%/%TOTAL% Test(s) Failed
)
echo ========================================

exit /b %FAILED%
