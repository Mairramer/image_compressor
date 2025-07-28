# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10.2)

project(image_compressor CXX)

add_library(image_compressor SHARED
    src/image_compressor.cpp
)


set_target_properties(image_compressor PROPERTIES
    CXX_STANDARD 11
    POSITION_INDEPENDENT_CODE ON
)

# Debug flags: Slight optimization and includes debug symbols
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -g")

# Release flags: Maximum optimization, no debug symbols
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

target_include_directories(image_compressor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (ANDROID)
    message(STATUS "Configuring for Android")
    # target_compile_definitions(image_compressor PRIVATE ANDROID_BUILD)
elseif(IOS)
    message(STATUS "Configuring for iOS")
    # target_compile_definitions(image_compressor PRIVATE IOS_BUILD)
endif()
