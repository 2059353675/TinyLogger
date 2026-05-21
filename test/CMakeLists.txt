# TinyLogger Tests CMakeLists.txt
#
# NOTE: This file is intended to be used as a subdirectory of the main CMakeLists.txt only.
# Do not build this directory standalone.
#
# To build tests, use the main build system:
#   cmake -B build -DCMAKE_BUILD_TYPE=Debug -DTINYLOGGER_BUILD_TESTS=ON
#   cmake --build build --config Debug
#   ctest --test-dir build --output-on-failure -C Debug

# ==================== Test Executables ====================

# RingBuffer test
add_executable(test_ring_buffer test_ring_buffer.cpp)
target_link_libraries(test_ring_buffer PRIVATE TinyLogger::tinylogger)

# Printer test
add_executable(test_printer test_printer.cpp)
target_link_libraries(test_printer PRIVATE TinyLogger::tinylogger)

# Distributor test
add_executable(test_distributor test_distributor.cpp)
target_link_libraries(test_distributor PRIVATE TinyLogger::tinylogger)

# Logger integration test
add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE TinyLogger::tinylogger)

# ==================== Test Configuration ====================

add_test(NAME RingBufferTest COMMAND test_ring_buffer)
add_test(NAME PrinterTest COMMAND test_printer)
add_test(NAME DistributorTest COMMAND test_distributor)
add_test(NAME LoggerIntegrationTest COMMAND test_logger)

# Custom target: run all tests
add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS test_ring_buffer test_printer test_distributor test_logger
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)