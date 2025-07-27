enable_testing()

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(test_compress_image
    test_compress_image.cpp
)

target_link_libraries(test_compress_image
    compress_image
    GTest::gtest_main
)

gtest_discover_tests(test_compress_image)
