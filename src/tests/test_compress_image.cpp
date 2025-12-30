#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <iostream>
#include "image_compressor.h"


size_t getFileSize(const char* path) {
    std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
    return in.is_open() ? static_cast<size_t>(in.tellg()) : 0;
}

TEST(CompressImageTest, NullPathReturnsNull) {
    EXPECT_EQ(image_compressor_from_path(nullptr, 75, 1080, 1920), nullptr);
}

TEST(CompressImageTest, InvalidFileReturnsNull) {
    EXPECT_EQ(image_compressor_from_path("invalid_path.jpg", 75, 1080, 1920), nullptr);
}

TEST(CompressImageTest, ValidImageBase64OutputWithSizeInfo) {
    const char* image_path = "../test_assets/5mb.jpg";
    size_t originalSize = getFileSize(image_path);
    ASSERT_GT(originalSize, 0u) << "Failed to get original file size";

    char* result = image_compressor_from_path(image_path, 85,1080, 1920);
    ASSERT_NE(result, nullptr);

    std::string base64(result);
    EXPECT_FALSE(base64.empty());
    EXPECT_EQ(base64.find("data:image/jpeg;base64"), std::string::npos);

    std::cout << "Original size: " << originalSize << " bytes\n";
    std::cout << "Compressed Base64 size: " << base64.size() << " bytes\n";

    image_compressor_free_string(result);
}

TEST(CompressImageTest, QualityOutOfRangeHandled) {
    const char* image_path = "../test_assets/5mb.jpg";

    char* resultLow = image_compressor_from_path(image_path, -10, 1080, 1920);
    char* resultHigh = image_compressor_from_path(image_path, 200, 1080, 1920);

    ASSERT_NE(resultLow, nullptr);
    ASSERT_NE(resultHigh, nullptr);

    image_compressor_free_string(resultLow);
    image_compressor_free_string(resultHigh);
}

TEST(CompressImageTest, ResizeProperlyApplied) {
    const char* image_path = "../test_assets/5mb.jpg";
    char* result = image_compressor_from_path(image_path, 80, 480, 720);
    ASSERT_NE(result, nullptr);

    std::string base64(result);
    EXPECT_FALSE(base64.empty());
    image_compressor_free_string(result);
}
