cmake_minimum_required(VERSION 3.14)
project(TinyLoggerTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用系统 nlohmann/json 库（位于 /usr/include/nlohmann/json.hpp）
find_path(NLOHMANN_JSON_INCLUDE_DIR NAMES nlohmann/json.hpp PATHS /usr/include)
if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    message(FATAL_ERROR "nlohmann/json.hpp not found in /usr/include")
endif()

# 使用系统 fmt 库（位于 /usr/include/fmt/）
find_path(FMT_INCLUDE_DIR NAMES fmt/format.h PATHS /usr/include)
if(NOT FMT_INCLUDE_DIR)
    message(FATAL_ERROR "fmt/format.h not found in /usr/include")
endif()
find_library(FMT_LIBRARY NAMES fmt)
if(NOT FMT_LIBRARY)
    message(FATAL_ERROR "libfmt not found")
endif()

# 包含头文件（项目根目录的 include + nlohmann/json + fmt）
include_directories(${CMAKE_SOURCE_DIR}/../include ${NLOHMANN_JSON_INCLUDE_DIR} ${FMT_INCLUDE_DIR})

# 源文件（相对于项目根目录）
set(TINYLOGGER_SOURCES
    ${CMAKE_SOURCE_DIR}/../src/ring_buffer.cpp
    ${CMAKE_SOURCE_DIR}/../src/config.cpp
    ${CMAKE_SOURCE_DIR}/../src/distributor.cpp
)

# ==================== 测试可执行文件 ====================

# RingBuffer 测试
add_executable(test_ring_buffer test_ring_buffer.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_ring_buffer ${FMT_LIBRARY})

# Config 测试
add_executable(test_config test_config.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_config ${FMT_LIBRARY})

# Printer 测试
add_executable(test_printer test_printer.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_printer ${FMT_LIBRARY})

# Distributor 测试
add_executable(test_distributor test_distributor.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_distributor ${FMT_LIBRARY})

# Logger 集成测试
add_executable(test_logger test_logger.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_logger ${FMT_LIBRARY})

# ==================== 测试配置 ====================

enable_testing()

add_test(NAME RingBufferTest COMMAND test_ring_buffer)
add_test(NAME ConfigTest COMMAND test_config)
add_test(NAME PrinterTest COMMAND test_printer)
add_test(NAME DistributorTest COMMAND test_distributor)
add_test(NAME LoggerIntegrationTest COMMAND test_logger)

# 自定义目标：运行所有测试
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_ring_buffer test_config test_printer test_distributor test_logger
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
