cmake_minimum_required(VERSION 3.14)
project(TinyLogger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 选项
option(TINYLOGGER_BUILD_TESTS "Build tests" ON)
option(TINYLOGGER_BUILD_EXAMPLES "Build examples" ON)

# ==================== 查找依赖 ====================

# nlohmann/json - 使用系统路径
find_path(NLOHMANN_JSON_INCLUDE_DIR NAMES nlohmann/json.hpp PATHS /usr/include /usr/local/include)
if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    message(FATAL_ERROR "nlohmann/json.hpp not found. Please install nlohmann-json package.")
endif()
add_library(nlohmann_json INTERFACE IMPORTED)
target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR})

# fmt - 使用系统路径
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

# ==================== 库定义 ====================

# 头文件目录
set(TINYLOGGER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 源文件
set(TINYLOGGER_SOURCES
    src/ring_buffer.cpp
    src/config.cpp
    src/distributor.cpp
)

# 创建静态库
add_library(tinylogger STATIC ${TINYLOGGER_SOURCES})
target_include_directories(tinylogger
    PUBLIC
        $<BUILD_INTERFACE:${TINYLOGGER_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(tinylogger
    PUBLIC
        fmt
    PRIVATE
        nlohmann_json
)

# 设置库属性
set_target_properties(tinylogger PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME TinyLogger
)

# ==================== 示例程序 ====================

if(TINYLOGGER_BUILD_EXAMPLES)
    add_executable(example examples/example.cpp)
    target_link_libraries(example PRIVATE tinylogger)
    
    # 复制配置文件到构建目录
    file(COPY examples/logger_config.json
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY examples/logger_config.json
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/examples)
endif()

# ==================== 测试 ====================

if(TINYLOGGER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# ==================== 安装 ====================

include(GNUInstallDirs)

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
include(CMakePackageConfigHelpers)

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
