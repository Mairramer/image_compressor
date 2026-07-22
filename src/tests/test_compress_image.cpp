#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "image_compressor.h"

size_t getFileSize(const char* path) {
    std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
    return in.is_open() ? static_cast<size_t>(in.tellg()) : 0;
}

std::vector<uint8_t> readFileBytes(const char* path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input),
                                std::istreambuf_iterator<char>());
}

TEST(CompressImageTest, NullPathReturnsNull) {
    EXPECT_EQ(image_compressor_from_path(nullptr, 75, 1080, 1920), nullptr);
}

TEST(CompressImageTest, InvalidFileReturnsNull) {
    EXPECT_EQ(image_compressor_from_path("invalid_path.jpg", 75, 1080, 1920), nullptr);
}

TEST(CompressImageTest, InvalidByteInputsReturnNull) {
    const uint8_t invalid_bytes[] = {0x00, 0x01, 0x02, 0x03};

    EXPECT_EQ(image_compressor_from_bytes(nullptr, 4, 75, 1080, 1920), nullptr);
    EXPECT_EQ(image_compressor_from_bytes(invalid_bytes, 0, 75, 1080, 1920), nullptr);
    EXPECT_EQ(image_compressor_from_bytes(invalid_bytes, 4, 75, 1080, 1920), nullptr);
}

TEST(CompressImageTest, PathAndBytesProduceSameOutput) {
    const char* image_path = "../test_assets/5mb.jpg";
    const std::vector<uint8_t> bytes = readFileBytes(image_path);
    ASSERT_FALSE(bytes.empty());

    char* path_result = image_compressor_from_path(image_path, 80, 480, 720);
    char* bytes_result =
        image_compressor_from_bytes(bytes.data(), static_cast<int>(bytes.size()), 80, 480, 720);

    ASSERT_NE(path_result, nullptr);
    ASSERT_NE(bytes_result, nullptr);
    EXPECT_STREQ(path_result, bytes_result);

    image_compressor_free_string(path_result);
    image_compressor_free_string(bytes_result);
}

TEST(CompressImageTest, TinyBoundsNeverProduceZeroDimension) {
    const char* image_path = "../test_assets/5mb.jpg";
    char* result = image_compressor_from_path(image_path, 75, 1, 1);

    ASSERT_NE(result, nullptr);
    EXPECT_NE(result[0], '\0');
    image_compressor_free_string(result);
}

TEST(CompressImageTest, ValidImageBase64OutputWithSizeInfo) {
    const char* image_path = "../test_assets/5mb.jpg";
    size_t originalSize = getFileSize(image_path);
    ASSERT_GT(originalSize, 0u) << "Failed to get original file size";

    char* result = image_compressor_from_path(image_path, 85, 1080, 1920);
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

TEST(CompressImageTest, GoldenTest) {
    const char* image_path = "../test_assets/5mb.jpg";
    const char* golden_path = "../test_assets/5mb_golden.txt";

    // Compress with specific parameters to ensure deterministic output
    char* result = image_compressor_from_path(image_path, 80, 480, 720);
    ASSERT_NE(result, nullptr);

    std::string base64(result);
    image_compressor_free_string(result);

    std::ifstream golden_in(golden_path);
    if (!golden_in.is_open() || std::getenv("UPDATE_GOLDENS")) {
        // Generate or update golden file
        std::ofstream golden_out(golden_path);
        ASSERT_TRUE(golden_out.is_open()) << "Failed to write golden file.";
        golden_out << base64;
        golden_out.close();
        std::cout << "Golden file generated/updated at " << golden_path << "\n";
    } else {
        // Compare with golden file
        std::string expected_base64((std::istreambuf_iterator<char>(golden_in)),
                                    std::istreambuf_iterator<char>());
        while (!expected_base64.empty() &&
               std::isspace(static_cast<unsigned char>(expected_base64.back()))) {
            expected_base64.pop_back();
        }
        EXPECT_EQ(base64, expected_base64)
            << "Compressed base64 output does not match golden file!";
    }
}
