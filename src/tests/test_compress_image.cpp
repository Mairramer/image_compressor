// #include <gtest/gtest.h>
// #include <string>
// #include <iostream>

// extern "C" {
//     char* compress_image(const char* path, int quality);
//     void free_compressed_image(char* ptr);
// }

// // Implementação simples para liberar memória alocada
// extern "C" void free_compressed_image(char* ptr) {
//     free(ptr);
// }

// TEST(CompressImageTest, NullPathReturnsNull) {
//     EXPECT_EQ(compress_image(nullptr, 75), nullptr);
// }

// TEST(CompressImageTest, InvalidFileReturnsNull) {
//     EXPECT_EQ(compress_image("invalid_path.jpg", 75), nullptr);
// }

// TEST(CompressImageTest, ValidImageBase64Output) {
//     // Caminho absoluto da imagem de teste (ajuste conforme seu sistema)
//     const char* image_path = "/Users/mairramersilva/Projects/Flutter/TG.totem-app-v3.FLUTTER/cpp/test_assets/5mb.jpg";

//     char* result = compress_image(image_path, 85);

//     if (result == nullptr) {
//         std::cerr << "Falha ao carregar/comprimir a imagem: " << image_path << std::endl;
//     }
//     ASSERT_NE(result, nullptr);

//     std::string base64(result);
//     EXPECT_FALSE(base64.empty());
//     EXPECT_TRUE(base64.find("data:image/jpeg;base64") == std::string::npos); // Deve ser base64 cru, sem prefixo MIME

//     free_compressed_image(result);
// }

// TEST(CompressImageTest, QualityOutOfRangeHandled) {
//     const char* image_path = "/Users/mairramersilva/Projects/Flutter/TG.totem-app-v3.FLUTTER/cpp/test_assets/5mb.jpg";

//     char* resultLow = compress_image(image_path, -10);
//     char* resultHigh = compress_image(image_path, 200);

//     ASSERT_NE(resultLow, nullptr);
//     ASSERT_NE(resultHigh, nullptr);

//     free_compressed_image(resultLow);
//     free_compressed_image(resultHigh);
// }


#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <iostream>

extern "C" {
    char* compress_image(const char* path, int quality);
    void free_compressed_image(char* ptr);
}

// Função auxiliar para obter o tamanho do arquivo original
size_t getFileSize(const char* path) {
    std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
    if (!in.is_open()) return 0;
    return static_cast<size_t>(in.tellg());
}

TEST(CompressImageTest, NullPathReturnsNull) {
    EXPECT_EQ(compress_image(nullptr, 75), nullptr);
}

TEST(CompressImageTest, InvalidFileReturnsNull) {
    EXPECT_EQ(compress_image("invalid_path.jpg", 75), nullptr);
}

TEST(CompressImageTest, ValidImageBase64OutputWithSizeInfo) {
    const char* image_path = "/Users/mairramersilva/Projects/Flutter/TG.totem-app-v3.FLUTTER/cpp/test_assets/5mb.jpg";

    size_t originalSize = getFileSize(image_path);
    ASSERT_GT(originalSize, 0u) << "Falha ao obter tamanho do arquivo original";

    char* result = compress_image(image_path, 85);
    ASSERT_NE(result, nullptr);

    std::string base64(result);
    EXPECT_FALSE(base64.empty());
    EXPECT_TRUE(base64.find("data:image/jpeg;base64") == std::string::npos);

    size_t compressedSize = base64.size();

    std::cout << "Tamanho original: " << originalSize << " bytes\n";
    std::cout << "Tamanho compactado (base64): " << compressedSize << " bytes\n";

    free_compressed_image(result);
}

TEST(CompressImageTest, QualityOutOfRangeHandled) {
    const char* image_path = "/Users/mairramersilva/Projects/Flutter/TG.totem-app-v3.FLUTTER/cpp/test_assets/5mb.jpg";

    // Se compress_image trata qualidades inválidas como 1 ou 100, teste espera resultado válido
    char* resultLow = compress_image(image_path, -10);
    char* resultHigh = compress_image(image_path, 200);

    ASSERT_NE(resultLow, nullptr);
    ASSERT_NE(resultHigh, nullptr);

    free_compressed_image(resultLow);
    free_compressed_image(resultHigh);
}
