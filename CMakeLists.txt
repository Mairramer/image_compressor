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

target_include_directories(image_compressor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Android/iOS configurações podem ir aqui se quiser
if (ANDROID)
    message(STATUS "Configuring for Android")
    # target_compile_definitions(image_compressor PRIVATE ANDROID_BUILD)
elseif(IOS)
    message(STATUS "Configuring for iOS")
    # target_compile_definitions(image_compressor PRIVATE IOS_BUILD)
endif()
