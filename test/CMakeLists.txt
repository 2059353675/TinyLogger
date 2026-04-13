# TinyLogger Tests CMakeLists.txt
#
# 注意：此文件仅作为主 CMakeLists.txt 的子目录使用
# 不应单独构建此目录
#
# 构建测试请使用主构建系统：
#   mkdir build && cd build
#   cmake ..
#   make run_tests

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==================== 包含头文件 ====================
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# 源文件
set(TINYLOGGER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ring_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/distributor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/printer_console.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/printer_file.cpp
)

# ==================== 测试可执行文件 ====================

# RingBuffer 测试
add_executable(test_ring_buffer test_ring_buffer.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_ring_buffer fmt nlohmann_json)

# Config 测试
add_executable(test_config test_config.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_config fmt nlohmann_json)

# Printer 测试
add_executable(test_printer test_printer.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_printer fmt nlohmann_json)

# Distributor 测试
add_executable(test_distributor test_distributor.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_distributor fmt nlohmann_json)

# Logger 集成测试
add_executable(test_logger test_logger.cpp ${TINYLOGGER_SOURCES})
target_link_libraries(test_logger fmt nlohmann_json)

# ==================== 测试配置 ====================

# 注意：enable_testing() 已在根 CMakeLists.txt 中调用

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
