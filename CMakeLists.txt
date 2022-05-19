cmake_minimum_required(VERSION 3.5)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)

# Set the project name and version number.
project(gencrypto VERSION 0.1.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_C_FLAGS "-Wall -Wextra -DHAVE_CONFIG_H ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -DHAVE_CONFIG_H ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Require the c99 standard to compile C code.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Require the c++11 standard to compile C++ code.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Export configuration options.
configure_file(config.h.in config.h)

# Add the source directory to the include path.
include_directories(src)

# Add the subdirectories.
add_subdirectory(src)
add_subdirectory(test)

# Enable testing support.
enable_testing()

# Custom 'test' rule to run the unit tests in a more verbose way.
add_custom_target(test-verbose COMMAND ${CMAKE_CTEST_COMMAND}
    --force-new-ctest-process
    --verbose
    --output-on-failure
)
