/**
 * @file image_compressor.cpp
 * @brief A collection of utility functions for loading, processing, and encoding images.
 *
 * This file provides a complete pipeline for image manipulation, including:
 * - Loading an image from a file path.
 * - Reading EXIF orientation data to correctly rotate/flip the image.
 * - Resizing the image to fit within specified maximum dimensions while preserving aspect ratio.
 * - Compressing the image into the JPEG format with a given quality.
 * - Encoding the final JPEG data into a Base64 string.
 *
 * It is designed to be called from other languages (e.g., via FFI), providing a C-style interface
 * with `image_compressor_from_path` as the main entry point and `image_compressor_free_string`
 * for memory management.
 *
 * @dependencies
 * - stb_image.h: for loading images.
 * - stb_image_write.h: for writing/compressing images to JPEG.
 * - stb_image_resize2.h: for high-quality image resizing.
 */
#include "image_compressor.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>  // malloc, free
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include "stb/stb_image_resize2.h"

// ---------------------------------------------------------------------------
// Base64
// ---------------------------------------------------------------------------

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

/**
 * @brief Encodes a byte array into a Base64 string.
 *
 * @param bytes_to_encode Pointer to data to encode.
 * @param in_len          Length of the data in bytes.
 * @return Base64 encoded string.
 */
static std::string base64_encode(const unsigned char* bytes_to_encode, size_t in_len) {
    std::string ret;
    // Pre-allocate the exact output size to avoid repeated reallocations.
    ret.reserve(((in_len + 2) / 3) * 4);

    int i = 0;
    unsigned char char_array_3[3], char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i > 0) {
        for (int j = i; j < 3; j++) char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; j < i + 1; j++) ret += base64_chars[char_array_4[j]];

        while ((i++ < 3)) ret += '=';
    }

    return ret;
}

// ---------------------------------------------------------------------------
// Image library callbacks and wrappers
// ---------------------------------------------------------------------------

/**
 * @brief Callback for stbi_write_jpg_to_func — appends JPEG chunks into a vector.
 */
static void write_jpg_to_vector(void* context, void* data, int size) {
    if (size <= 0) return;
    auto* buffer = reinterpret_cast<std::vector<unsigned char>*>(context);
    const auto* bytes = static_cast<const unsigned char*>(data);
    buffer->insert(buffer->end(), bytes, bytes + size);
}

/**
 * @brief Resizes an image using the stb_image_resize2 library.
 *
 * @param input    Pointer to the source raw image data.
 * @param input_w  Width of the source image.
 * @param input_h  Height of the source image.
 * @param channels Number of color channels (1–4).
 * @param output   Vector that will receive the resized image data.
 * @param output_w Target width.
 * @param output_h Target height.
 * @return true on success, false if the channel count is unsupported or the
 *         resize library returns an error.
 */
static bool resize_image(const unsigned char* input, int input_w, int input_h, int channels,
                         std::vector<unsigned char>& output, int output_w, int output_h) {
    stbir_pixel_layout layout;
    switch (channels) {
        case 1:
            layout = STBIR_1CHANNEL;
            break;
        case 2:
            layout = STBIR_2CHANNEL;
            break;
        case 3:
            layout = STBIR_RGB;
            break;
        case 4:
            layout = STBIR_RGBA;
            break;
        default:
            return false;
    }

    output.resize(static_cast<size_t>(output_w) * output_h * channels);

    unsigned char* result = stbir_resize_uint8_srgb(input, input_w, input_h, 0, output.data(),
                                                    output_w, output_h, 0, layout);

    return result != nullptr;
}

/**
 * @brief Compresses raw pixel data to JPEG and appends into @p buffer.
 *
 * @return true on success, false otherwise.
 */
static bool compress_to_jpeg_buffer(const unsigned char* image_data, int width, int height,
                                    int channels, int quality, std::vector<unsigned char>& buffer) {
    return stbi_write_jpg_to_func(write_jpg_to_vector, &buffer, width, height, channels, image_data,
                                  quality) != 0;
}

// ---------------------------------------------------------------------------
// EXIF orientation
// ---------------------------------------------------------------------------

/**
 * @brief Reads the EXIF orientation tag from a JPEG file.
 *
 * Parses the JPEG APP1/EXIF segment to extract the orientation value.
 * Returns 1 (normal) on any parse error or if no orientation tag is found.
 *
 * @param filename Path to the JPEG file.
 * @return Orientation value 1–8, or 1 if unavailable.
 */
static int read_exif_orientation(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return 1;

    // Verify JPEG SOI marker.
    uint8_t marker[2];
    if (fread(marker, 1, 2, f) != 2 || marker[0] != 0xFF || marker[1] != 0xD8) {
        fclose(f);
        return 1;
    }

    int orientation = 1;

    while (!feof(f)) {
        if (fread(marker, 1, 2, f) != 2) break;
        if (marker[0] != 0xFF) break;

        uint8_t segment_type = marker[1];

        uint8_t size_buf[2];
        if (fread(size_buf, 1, 2, f) != 2) break;
        uint16_t segment_size = static_cast<uint16_t>((size_buf[0] << 8) | size_buf[1]);

        // segment_size includes the 2 size bytes themselves; payload = size - 2.
        if (segment_size < 2) break;
        uint16_t payload_size = segment_size - 2;

        if (segment_type == 0xE1) {  // APP1 — may contain EXIF
            // Allocate with null check.
            std::vector<uint8_t> data(payload_size);
            if (fread(data.data(), 1, payload_size, f) != payload_size) break;

            // Need at least "Exif\0\0" (6) + TIFF header (8) to be useful.
            if (payload_size < 14) break;
            if (memcmp(data.data(), "Exif\0\0", 6) != 0) break;

            const uint8_t* tiff = data.data() + 6;
            const size_t tiff_size = payload_size - 6;

            bool is_be = false;
            if (memcmp(tiff, "MM", 2) == 0)
                is_be = true;
            else if (memcmp(tiff, "II", 2) == 0)
                is_be = false;
            else
                break;

            auto read16 = [&](const uint8_t* p) -> uint16_t {
                return is_be ? static_cast<uint16_t>((p[0] << 8) | p[1])
                             : static_cast<uint16_t>(p[0] | (p[1] << 8));
            };
            auto read32 = [&](const uint8_t* p) -> uint32_t {
                return is_be ? (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
                                   (uint32_t(p[2]) << 8) | p[3]
                             : uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
                                   (uint32_t(p[3]) << 24);
            };

            // Bounds-check the IFD0 offset (offset is relative to start of TIFF header).
            uint32_t ifd0_offset = read32(tiff + 4);
            // IFD entry list needs: 2-byte count + at least 1 × 12-byte entry.
            if (ifd0_offset + 2 + 12 > tiff_size) break;

            const uint8_t* ifd0 = tiff + ifd0_offset;
            uint16_t entries = read16(ifd0);
            const uint8_t* entry_ptr = ifd0 + 2;

            // Validate that all entries fit within the buffer.
            if (ifd0_offset + 2 + static_cast<uint32_t>(entries) * 12 > tiff_size) {
                entries = static_cast<uint16_t>((tiff_size - ifd0_offset - 2) / 12);
            }

            for (uint16_t i = 0; i < entries; ++i, entry_ptr += 12) {
                uint16_t tag = read16(entry_ptr);
                if (tag == 0x0112) {  // Orientation
                    uint16_t format = read16(entry_ptr + 2);
                    uint32_t components = read32(entry_ptr + 4);
                    if (format == 3 && components == 1) {  // SHORT, 1 value
                        uint16_t val = read16(entry_ptr + 8);
                        if (val >= 1 && val <= 8) orientation = val;
                    }
                    break;  // Found the tag; no need to scan further.
                }
            }
            break;  // APP1 processed.
        } else {
            // Skip this segment's payload.
            if (fseek(f, payload_size, SEEK_CUR) != 0) break;
        }
    }

    fclose(f);
    return orientation;
}

// ---------------------------------------------------------------------------
// Pixel-level transformations
// ---------------------------------------------------------------------------

/**
 * @brief Rotates an image 90 degrees clockwise.
 *
 * Output dimensions are height × width (transposed).
 */
static void rotate90(const unsigned char* src, unsigned char* dst, int width, int height,
                     int channels) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int src_i = (y * width + x) * channels;
            const int dst_i = (x * height + (height - y - 1)) * channels;
            std::memcpy(dst + dst_i, src + src_i, channels);
        }
    }
}

/**
 * @brief Rotates an image 180 degrees in-place (src → dst, same dimensions).
 *
 * Pixels are reversed end-to-end in steps of `channels` so that
 * individual channel bytes within each pixel are preserved correctly.
 */
static void rotate180(const unsigned char* src, unsigned char* dst, int width, int height,
                      int channels) {
    const int total_pixels = width * height;
    for (int i = 0; i < total_pixels; ++i) {
        const int src_i = i * channels;
        const int dst_i = (total_pixels - 1 - i) * channels;
        std::memcpy(dst + dst_i, src + src_i, channels);
    }
}

/**
 * @brief Rotates an image 270 degrees clockwise.
 *
 * Output dimensions are height × width (transposed).
 */
static void rotate270(const unsigned char* src, unsigned char* dst, int width, int height,
                      int channels) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int src_i = (y * width + x) * channels;
            const int dst_i = ((width - x - 1) * height + y) * channels;
            std::memcpy(dst + dst_i, src + src_i, channels);
        }
    }
}

/**
 * @brief Flips an image horizontally (mirror left ↔ right).
 */
static void flip_horizontal(const unsigned char* src, unsigned char* dst, int width, int height,
                            int channels) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int src_i = (y * width + x) * channels;
            const int dst_i = (y * width + (width - x - 1)) * channels;
            std::memcpy(dst + dst_i, src + src_i, channels);
        }
    }
}

/**
 * @brief Scales down @p original dimensions to fit inside (@p max_w, @p max_h)
 *        while preserving aspect ratio. Never upscales.
 */
static void fit_inside_box(int original_w, int original_h, int max_w, int max_h, int& out_w,
                           int& out_h) {
    const float ratio_w = static_cast<float>(max_w) / original_w;
    const float ratio_h = static_cast<float>(max_h) / original_h;
    const float ratio = (ratio_w < ratio_h) ? ratio_w : ratio_h;

    if (ratio >= 1.0f) {
        out_w = original_w;
        out_h = original_h;
    } else {
        out_w = static_cast<int>(original_w * ratio);
        out_h = static_cast<int>(original_h * ratio);
    }
}

// ---------------------------------------------------------------------------
// Shared encode-to-base64 finalisation
// Avoids duplicating the malloc / memcpy / null-check block in both APIs.
// ---------------------------------------------------------------------------

static char* encode_jpeg_to_base64_cstr(const std::vector<unsigned char>& jpeg_buffer) {
    std::string base64 = base64_encode(jpeg_buffer.data(), jpeg_buffer.size());
    char* result = static_cast<char*>(malloc(base64.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, base64.c_str(), base64.size());
    result[base64.size()] = '\0';
    return result;
}

// ---------------------------------------------------------------------------
// Public C-style API
// ---------------------------------------------------------------------------

/**
 * @brief Loads, EXIF-corrects, resizes and JPEG-compresses an image from a file path.
 *
 * Steps performed:
 *   1. Load the image via stb_image.
 *   2. Read EXIF orientation and apply the necessary rotation/flip.
 *   3. Scale down to fit within (max_width × max_height), preserving aspect ratio.
 *   4. Compress to JPEG at the requested quality.
 *   5. Return the result as a Base64-encoded, null-terminated C string.
 *
 * @param path       File path of the source image.
 * @param quality    JPEG quality (1–100; clamped automatically).
 * @param max_width  Maximum output width.  Pass 0 for no width constraint.
 * @param max_height Maximum output height. Pass 0 for no height constraint.
 * @return Heap-allocated Base64 C string, or nullptr on failure.
 *         Caller must free it with image_compressor_free_string().
 */
extern "C" char* image_compressor_from_path(const char* path, int quality, int max_width,
                                            int max_height) {
    try {
        if (!path) return nullptr;

        quality = std::clamp(quality, 1, 100);

        int width, height, channels;
        unsigned char* img = stbi_load(path, &width, &height, &channels, 0);
        if (!img) return nullptr;

        const int orientation = read_exif_orientation(path);

        // Apply EXIF rotation / flip.
        std::vector<unsigned char> rotated_buf;
        const unsigned char* img_to_use = img;
        int w = width, h = height;

        if (orientation != 1) {
            const size_t pixel_count = static_cast<size_t>(width) * height * channels;
            rotated_buf.resize(pixel_count);

            switch (orientation) {
                case 2:
                    flip_horizontal(img, rotated_buf.data(), width, height, channels);
                    break;
                case 3:
                    rotate180(img, rotated_buf.data(), width, height, channels);
                    break;
                case 6:
                    rotate90(img, rotated_buf.data(), width, height, channels);
                    std::swap(w, h);
                    break;
                case 8:
                    rotate270(img, rotated_buf.data(), width, height, channels);
                    std::swap(w, h);
                    break;
                default:
                    rotated_buf.clear();  // Unsupported orientation — use original.
                    break;
            }

            if (!rotated_buf.empty()) img_to_use = rotated_buf.data();
        }

        // Compute output dimensions using the shared helper.
        int new_w = w, new_h = h;
        {
            const int limit_w = (max_width > 0) ? max_width : w;
            const int limit_h = (max_height > 0) ? max_height : h;
            fit_inside_box(w, h, limit_w, limit_h, new_w, new_h);
        }

        std::vector<unsigned char> resized_buf;
        if (new_w != w || new_h != h) {
            if (!resize_image(img_to_use, w, h, channels, resized_buf, new_w, new_h)) {
                stbi_image_free(img);
                return nullptr;
            }
            img_to_use = resized_buf.data();
            w = new_w;
            h = new_h;
        }

        std::vector<unsigned char> jpeg_buf;
        const bool ok = compress_to_jpeg_buffer(img_to_use, w, h, channels, quality, jpeg_buf);
        stbi_image_free(img);
        if (!ok) return nullptr;

        return encode_jpeg_to_base64_cstr(jpeg_buf);
    } catch (...) {
        return nullptr;
    }
}

/**
 * @brief Frees a string previously returned by image_compressor_from_path or
 *        image_compressor_from_bytes.
 */
extern "C" void image_compressor_free_string(char* ptr) { free(ptr); }

/**
 * @brief Loads, resizes and JPEG-compresses an image from a raw byte buffer.
 *
 * EXIF orientation is intentionally skipped because bytes coming from Flutter
 * are already correctly oriented by the rendering pipeline.
 *
 * @param input_bytes Pointer to the input buffer.
 * @param input_size  Size of the input buffer in bytes.
 * @param quality     JPEG quality (1–100; clamped automatically).
 * @param max_width   Maximum output width.  Pass 0 for no width constraint.
 * @param max_height  Maximum output height. Pass 0 for no height constraint.
 * @return Heap-allocated Base64 C string, or nullptr on failure.
 *         Caller must free it with image_compressor_free_string().
 */
extern "C" char* image_compressor_from_bytes(const uint8_t* input_bytes, int input_size,
                                             int quality, int max_width, int max_height) {
    try {
        if (!input_bytes || input_size <= 0) return nullptr;

        quality = std::clamp(quality, 1, 100);

        int width, height, channels;
        unsigned char* img =
            stbi_load_from_memory(input_bytes, input_size, &width, &height, &channels, 0);
        if (!img) return nullptr;

        int w = width, h = height;
        int new_w = w, new_h = h;

        const int limit_w = (max_width > 0) ? max_width : w;
        const int limit_h = (max_height > 0) ? max_height : h;
        fit_inside_box(w, h, limit_w, limit_h, new_w, new_h);

        const unsigned char* img_to_use = img;
        std::vector<unsigned char> resized_buf;

        if (new_w != w || new_h != h) {
            if (!resize_image(img, w, h, channels, resized_buf, new_w, new_h)) {
                stbi_image_free(img);
                return nullptr;
            }
            img_to_use = resized_buf.data();
            w = new_w;
            h = new_h;
        }

        std::vector<unsigned char> jpeg_buf;
        const bool ok = compress_to_jpeg_buffer(img_to_use, w, h, channels, quality, jpeg_buf);
        stbi_image_free(img);
        if (!ok) return nullptr;

        return encode_jpeg_to_base64_cstr(jpeg_buf);
    } catch (...) {
        return nullptr;
    }
}
