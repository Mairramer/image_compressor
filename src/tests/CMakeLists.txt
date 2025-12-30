cmake_minimum_required(VERSION 3.10)
project(image_compressor_tests CXX)

enable_testing()

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(test_compress_image
    test_compress_image.cpp
    ../image_compressor.cpp
)

target_include_directories(test_compress_image
    PRIVATE
        ..
        ../../third_party
)

target_link_libraries(test_compress_image
    GTest::gtest_main
)

gtest_discover_tests(test_compress_image)
