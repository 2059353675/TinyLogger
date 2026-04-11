cmake_minimum_required(VERSION 3.14)
project(TinyLoggerTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==================== 查找依赖 ====================

# nlohmann/json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    find_path(NLOHMANN_JSON_INCLUDE_DIR NAMES nlohmann/json.hpp PATHS /usr/include /usr/local/include)
    if(NOT NLOHMANN_JSON_INCLUDE_DIR)
        message(FATAL_ERROR "nlohmann/json.hpp not found. Please install nlohmann-json package.")
    endif()
    add_library(nlohmann_json INTERFACE IMPORTED)
    target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR})
endif()

# fmt
find_package(fmt QUIET)
if(NOT fmt_FOUND)
    find_path(FMT_INCLUDE_DIR NAMES fmt/format.h PATHS /usr/include /usr/local/include)
    find_library(FMT_LIBRARY NAMES fmt PATHS /usr/lib /usr/local/lib)
    if(NOT FMT_INCLUDE_DIR OR NOT FMT_LIBRARY)
        message(FATAL_ERROR "fmt library not found. Please install libfmt-dev package.")
    endif()
    add_library(fmt UNKNOWN IMPORTED)
    set_target_properties(fmt PROPERTIES
        IMPORTED_LOCATION ${FMT_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${FMT_INCLUDE_DIR}
    )
endif()

# ==================== 包含头文件 ====================
include_directories(${CMAKE_SOURCE_DIR}/../include)

# 源文件
set(TINYLOGGER_SOURCES
    ${CMAKE_SOURCE_DIR}/../src/ring_buffer.cpp
    ${CMAKE_SOURCE_DIR}/../src/config.cpp
    ${CMAKE_SOURCE_DIR}/../src/distributor.cpp
)

# ==================== 测试可执行文件 ====================

# RingBuffer 测试
add_executable(test_ring_buffer test_ring_buffer.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_ring_buffer fmt nlohmann_json::nlohmann_json)

# Config 测试
add_executable(test_config test_config.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_config fmt nlohmann_json::nlohmann_json)

# Printer 测试
add_executable(test_printer test_printer.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_printer fmt nlohmann_json::nlohmann_json)

# Distributor 测试
add_executable(test_distributor test_distributor.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_distributor fmt nlohmann_json::nlohmann_json)

# Logger 集成测试
add_executable(test_logger test_logger.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_logger fmt nlohmann_json::nlohmann_json)

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
