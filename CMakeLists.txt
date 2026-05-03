# =============================================================================
# 项目配置 / Project Configuration
# =============================================================================

# 指定最低CMake版本要求 / Specify minimum required CMake version
cmake_minimum_required(VERSION 3.14)

# 定义项目名称、版本及编程语言 / Define project name, version, and languages
project(TinyLogger VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准及强制要求 / Set C++ standard and enforce requirement
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 引入GNU安装目录规范及CMake包配置辅助模块 / Include GNU install directories and CMake package config helpers
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)


# =============================================================================
# 构建选项 / Build Options
# =============================================================================

# 是否构建测试用例（默认开启） / Whether to build tests (default: ON)
option(TINYLOGGER_BUILD_TESTS "Build tests" ON)

# 是否构建示例程序（默认开启） / Whether to build example programs (default: ON)
option(TINYLOGGER_BUILD_EXAMPLES "Build examples" ON)

# 是否使用系统已安装的fmt库而非通过FetchContent获取（默认关闭） / 
# Whether to use system-installed fmt library instead of FetchContent (default: OFF)
option(USE_SYSTEM_FMT "Use system-installed fmt instead of FetchContent" OFF)


# =============================================================================
# 依赖管理 / Dependency Management
# =============================================================================

include(FetchContent)

# 条件判断：使用系统fmt库或通过Git自动获取 / Conditional: use system fmt or fetch via Git
if(USE_SYSTEM_FMT)
    # 尝试查找系统已安装的fmt包 / Attempt to find system-installed fmt package
    find_package(fmt QUIET)
    # 若未找到则报错终止 / Terminate with error if not found
    if(NOT fmt_FOUND)
        message(FATAL_ERROR "fmt is required but could not be found")
    endif()
else()
    # 声明通过FetchContent获取fmt库的配置 / Declare FetchContent configuration for fmt
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 10.2.1  # 指定稳定版本 / Specify stable version
    )
    # 使fmt库对当前项目可用 / Make fmt library available to the project
    FetchContent_MakeAvailable(fmt)
endif()


# =============================================================================
# 库目标定义 / Library Target Definition
# =============================================================================

# 设置头文件包含根目录 / Set root directory for header files
set(TINYLOGGER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 定义库的源文件列表 / Define source file list for the library
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

# 创建静态库目标 / Create static library target
add_library(tinylogger STATIC ${TINYLOGGER_SOURCES})
# 定义命名空间别名，便于其他项目引用 / Define namespaced alias for external usage
add_library(TinyLogger::tinylogger ALIAS tinylogger)

# 设置目标包含目录：构建时与安装时使用不同路径 / Set include directories: different paths for build vs install
target_include_directories(tinylogger
        PUBLIC
        $<BUILD_INTERFACE:${TINYLOGGER_INCLUDE_DIR}>   # 构建阶段使用源码目录 / Use source dir during build
        $<INSTALL_INTERFACE:include>                    # 安装后使用标准include目录 / Use standard include dir after install
)

# 链接依赖库：公开链接fmt库，使用者会自动继承 / Link dependencies: publicly link fmt, consumers inherit automatically
target_link_libraries(tinylogger
        PUBLIC
        fmt::fmt
)

# 配置库目标属性 / Configure library target properties
set_target_properties(tinylogger PROPERTIES
        VERSION ${PROJECT_VERSION}                      # 设置库版本号 / Set library version
        SOVERSION ${PROJECT_VERSION_MAJOR}              # 设置SO版本号（用于动态库兼容性） / Set SO version for ABI compatibility
        POSITION_INDEPENDENT_CODE ON                    # 启用位置无关代码（便于静态库被动态库链接） / Enable PIC for static lib linking
        OUTPUT_NAME TinyLogger                          # 设置输出文件名 / Set output filename
        CXX_VISIBILITY_PRESET hidden                    # 默认隐藏符号，减少导出污染 / Hide symbols by default to reduce export pollution
        VISIBILITY_INLINES_HIDDEN ON                    # 内联函数符号也隐藏 / Hide inline function symbols as well
)

# 根据编译器类型设置警告选项 / Set compiler warning flags based on compiler type
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tinylogger PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(tinylogger PRIVATE /W4)
endif()


# =============================================================================
# 示例程序构建 / Example Programs Build
# =============================================================================

if(TINYLOGGER_BUILD_EXAMPLES)
    # 构建基础示例程序 / Build basic example program
    add_executable(example examples/example.cpp)
    target_link_libraries(example PRIVATE tinylogger)

    # 构建性能基准测试示例 / Build performance benchmark example
    add_executable(benchmark examples/benchmark.cpp)
    target_link_libraries(benchmark PRIVATE tinylogger)
endif()


# =============================================================================
# 测试配置 / Test Configuration
# =============================================================================

if(TINYLOGGER_BUILD_TESTS)
    # 启用CMake测试功能 / Enable CTest testing framework
    enable_testing()
    # 添加测试子目录（含测试用例定义） / Add test subdirectory containing test definitions
    add_subdirectory(test)
endif()


# =============================================================================
# 安装规则 / Installation Rules
# =============================================================================

# 安装头文件目录 / Install header files directory
install(DIRECTORY include/tiny_logger
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 安装库文件并导出目标 / Install library files and export targets
install(TARGETS tinylogger
        EXPORT TinyLoggerTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}    # 静态库安装路径 / Static library install path
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}    # 动态库安装路径 / Shared library install path
)

# 安装CMake目标导出文件（含命名空间） / Install CMake targets export file with namespace
install(EXPORT TinyLoggerTargets
        FILE TinyLoggerTargets.cmake
        NAMESPACE TinyLogger::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyLogger
)

# 生成并安装包版本配置文件 / Generate and install package version config file
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/TinyLoggerConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion  # 仅兼容相同主版本 / Compatible only with same major version
)

# 配置并生成主包配置文件 / Configure and generate main package config file
configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TinyLoggerConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/TinyLoggerConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyLogger
)

# 安装生成的CMake配置文件 / Install generated CMake config files
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/TinyLoggerConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/TinyLoggerConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyLogger
)


# =============================================================================
# 自定义清理目标 / Custom Clean Target
# =============================================================================

# 添加 clean-all 目标：清理构建产物 + 测试临时文件 + 示例产物
# Add clean-all target: clean build artifacts + test temp files + example outputs
add_custom_target(clean-all
        # 清理标准构建产物 / Clean standard build artifacts
        COMMAND ${CMAKE_BUILD_TOOL} clean

        # 清理测试相关临时文件 / Clean test-related temporary files
        COMMAND ${CMAKE_COMMAND} -E echo "Cleaning test temporary files..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_SOURCE_DIR}/test/build_test"
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/test/test_temp_*.json
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/test/test_temp_*.log
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/test/test_*.log

        # 清理示例程序产物 / Clean example program outputs
        COMMAND ${CMAKE_COMMAND} -E echo "Cleaning example temporary files..."
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/examples/example
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.log

        # 完成提示 / Completion message
        COMMAND ${CMAKE_COMMAND} -E echo "Clean-all complete."

        # 设置工作目录及注释 / Set working directory and display comment
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Cleaning all build artifacts and temporary files"
)