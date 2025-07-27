#include "image_compressor.h"
#include <cstring>
#include <vector>
#include <cstdlib> // malloc, free

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include "stb_image_resize2.h"

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string base64_encode(const unsigned char* bytes_to_encode, size_t in_len) {
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3], char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] =  (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                              ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                              ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] =   char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i > 0) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] =  (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                          ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                          ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] =   char_array_3[2] & 0x3f;

        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

static void write_jpg_to_vector(void* context, void* data, int size) {
    auto* buffer = reinterpret_cast<std::vector<unsigned char>*>(context);
    buffer->insert(buffer->end(), (unsigned char*)data, (unsigned char*)data + size);
}

static bool resize_image(
    const unsigned char* input,
    int input_w, int input_h, int channels,
    std::vector<unsigned char>& output,
    int output_w, int output_h
) {
    output.resize(output_w * output_h * channels);

    stbir_pixel_layout layout;
    if (channels == 1) layout = STBIR_1CHANNEL;
    else if (channels == 2) layout = STBIR_2CHANNEL;
    else if (channels == 3) layout = STBIR_RGB;
    else if (channels == 4) layout = STBIR_RGBA;
    else return false;

    unsigned char* result = stbir_resize_uint8_srgb(
        input,
        input_w,
        input_h,
        0,
        output.data(),
        output_w,
        output_h,
        0,
        layout
    );

    return result != nullptr;
}

static bool compress_to_jpeg_buffer(
    const unsigned char* image_data,
    int width, int height, int channels, int quality,
    std::vector<unsigned char>& buffer
) {
    return stbi_write_jpg_to_func(write_jpg_to_vector, &buffer, width, height, channels, image_data, quality) != 0;
}

extern "C" char* compress_image(const char* path, int quality, int max_size) {
    if (!path) return nullptr;

    if (quality < 1) quality = 1;
    if (quality > 100) quality = 100;

    int width, height, channels;
    unsigned char* img = stbi_load(path, &width, &height, &channels, 0);
    if (!img) return nullptr;

    int new_w = width;
    int new_h = height;

    if (max_size > 0 && (width > max_size || height > max_size)) {
        if (width >= height) {
            new_w = max_size;
            new_h = (int)((float)height * ((float)max_size / (float)width));
        } else {
            new_h = max_size;
            new_w = (int)((float)width * ((float)max_size / (float)height));
        }
    }

    const unsigned char* final_img = img;
    std::vector<unsigned char> resized_image;

    if (new_w != width || new_h != height) {
        if (!resize_image(img, width, height, channels, resized_image, new_w, new_h)) {
            stbi_image_free(img);
            return nullptr;
        }
        final_img = resized_image.data();
    }

    std::vector<unsigned char> jpeg_buffer;
    bool success = compress_to_jpeg_buffer(final_img, new_w, new_h, channels, quality, jpeg_buffer);
    stbi_image_free(img);
    if (!success) return nullptr;

    std::string base64 = base64_encode(jpeg_buffer.data(), jpeg_buffer.size());
    char* result = (char*)malloc(base64.size() + 1);
    if (!result) return nullptr;

    std::memcpy(result, base64.c_str(), base64.size());
    result[base64.size()] = '\0';

    return result;
}

extern "C" void free_compressed_image(char* ptr) {
    free(ptr);
}
