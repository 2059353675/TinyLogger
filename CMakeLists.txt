cmake_minimum_required(VERSION 3.14)
project(TinyLogger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 选项
option(TINYLOGGER_BUILD_TESTS "Build tests" ON)
option(TINYLOGGER_BUILD_EXAMPLES "Build examples" ON)

# ==================== 查找依赖 ====================

find_package(fmt REQUIRED)

# ==================== 库定义 ====================

# 头文件目录
set(TINYLOGGER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 源文件
set(TINYLOGGER_SOURCES
    src/logger.cpp
    src/logger_builder.cpp
    src/logger_factory.cpp
    src/ring_buffer.cpp
    src/distributor.cpp
    src/queue_registry.cpp
    src/console.cpp
    src/file.cpp
    src/null.cpp
)

# 创建静态库
add_library(tinylogger STATIC ${TINYLOGGER_SOURCES})
add_library(TinyLogger::tinylogger ALIAS tinylogger)

target_include_directories(tinylogger
    PUBLIC
        $<BUILD_INTERFACE:${TINYLOGGER_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(tinylogger
    PUBLIC
        fmt::fmt
)

# 设置库属性
set_target_properties(tinylogger PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME TinyLogger
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# 编译器警告
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tinylogger PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(tinylogger PRIVATE /W4)
endif()

# ==================== 示例程序 ====================

if(TINYLOGGER_BUILD_EXAMPLES)
    add_executable(example examples/example.cpp)
    target_link_libraries(example PRIVATE tinylogger)
    
    add_executable(speed_test examples/speed_test.cpp)
    target_link_libraries(speed_test PRIVATE tinylogger)
    
endif()

# ==================== 测试 ====================

if(TINYLOGGER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# ==================== 安装 ====================

# 安装头文件
install(DIRECTORY include/TinyLogger
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 安装库文件
install(TARGETS tinylogger
    EXPORT TinyLoggerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# 安装 CMake 配置文件
install(EXPORT TinyLoggerTargets
    FILE TinyLoggerTargets.cmake
    NAMESPACE TinyLogger::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyLogger
)

# 生成配置文件
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/TinyLoggerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TinyLoggerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/TinyLoggerConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyLogger
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/TinyLoggerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/TinyLoggerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyLogger
)

# ==================== 清理目标 ====================

# 添加 clean-all 目标：清理构建产物 + 测试临时文件 + 示例产物
add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning test temporary files..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_SOURCE_DIR}/test/build_test"
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/test/test_temp_*.json
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/test/test_temp_*.log
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/test/test_*.log
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning example temporary files..."
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/examples/example
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.log
    COMMAND ${CMAKE_COMMAND} -E echo "Clean-all complete."
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Cleaning all build artifacts and temporary files"
)
