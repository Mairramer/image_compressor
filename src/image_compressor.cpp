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
#include <cstdio>
#include <cstdlib>  // malloc, free
#include <cstring>
#include <limits>
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
static bool calculate_buffer_size(int width, int height, int channels, size_t& size) {
    if (width <= 0 || height <= 0 || channels <= 0) return false;

    const size_t width_size = static_cast<size_t>(width);
    const size_t height_size = static_cast<size_t>(height);
    const size_t channel_size = static_cast<size_t>(channels);
    const size_t max_size = std::numeric_limits<size_t>::max();

    if (width_size > max_size / height_size) return false;
    const size_t pixel_count = width_size * height_size;
    if (pixel_count > max_size / channel_size) return false;

    size = pixel_count * channel_size;
    return true;
}

static bool resize_image(const unsigned char* input, int input_w, int input_h, int channels,
                         std::vector<unsigned char>& output, int output_w, int output_h) {
    if (!input) return false;

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

    size_t output_size = 0;
    if (!calculate_buffer_size(output_w, output_h, channels, output_size)) return false;
    output.resize(output_size);

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
static bool marker_has_no_payload(uint8_t marker) {
    return marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7);
}

static uint16_t read_uint16(const uint8_t* data, bool big_endian) {
    return big_endian ? static_cast<uint16_t>((data[0] << 8) | data[1])
                      : static_cast<uint16_t>(data[0] | (data[1] << 8));
}

static uint32_t read_uint32(const uint8_t* data, bool big_endian) {
    if (big_endian) {
        return (uint32_t(data[0]) << 24) | (uint32_t(data[1]) << 16) |
               (uint32_t(data[2]) << 8) | uint32_t(data[3]);
    }
    return uint32_t(data[0]) | (uint32_t(data[1]) << 8) | (uint32_t(data[2]) << 16) |
           (uint32_t(data[3]) << 24);
}

/**
 * Parses a JPEG APP1 payload. Returns true when the payload is EXIF, even if
 * the orientation tag is missing or malformed. In that case orientation stays 1.
 */
static bool parse_exif_app1(const uint8_t* payload, size_t payload_size, int& orientation) {
    orientation = 1;
    if (!payload || payload_size < 14 || std::memcmp(payload, "Exif\0\0", 6) != 0) {
        return false;
    }

    const uint8_t* tiff = payload + 6;
    const size_t tiff_size = payload_size - 6;

    bool big_endian;
    if (std::memcmp(tiff, "MM", 2) == 0) {
        big_endian = true;
    } else if (std::memcmp(tiff, "II", 2) == 0) {
        big_endian = false;
    } else {
        return true;
    }

    if (read_uint16(tiff + 2, big_endian) != 42) return true;

    const size_t ifd0_offset = read_uint32(tiff + 4, big_endian);
    if (ifd0_offset > tiff_size || tiff_size - ifd0_offset < 2) return true;

    const uint8_t* ifd0 = tiff + ifd0_offset;
    const uint16_t declared_entries = read_uint16(ifd0, big_endian);
    const size_t available_entries = (tiff_size - ifd0_offset - 2) / 12;
    const size_t entries = std::min<size_t>(declared_entries, available_entries);

    const uint8_t* entry = ifd0 + 2;
    for (size_t index = 0; index < entries; ++index, entry += 12) {
        if (read_uint16(entry, big_endian) != 0x0112) continue;

        const uint16_t format = read_uint16(entry + 2, big_endian);
        const uint32_t components = read_uint32(entry + 4, big_endian);
        if (format == 3 && components == 1) {
            const uint16_t value = read_uint16(entry + 8, big_endian);
            if (value >= 1 && value <= 8) orientation = value;
        }
        break;
    }

    return true;
}

static int read_exif_orientation(const char* filename) {
    std::unique_ptr<FILE, decltype(&fclose)> file(fopen(filename, "rb"), fclose);
    if (!file) return 1;

    uint8_t marker_bytes[2];
    if (fread(marker_bytes, 1, 2, file.get()) != 2 || marker_bytes[0] != 0xFF ||
        marker_bytes[1] != 0xD8) {
        return 1;
    }

    while (true) {
        int prefix = fgetc(file.get());
        if (prefix == EOF || prefix != 0xFF) break;

        int marker_value;
        do {
            marker_value = fgetc(file.get());
        } while (marker_value == 0xFF);

        if (marker_value == EOF) break;
        const uint8_t marker = static_cast<uint8_t>(marker_value);
        if (marker == 0x00) continue;
        if (marker == 0xD9 || marker == 0xDA) break;
        if (marker_has_no_payload(marker)) continue;

        uint8_t size_bytes[2];
        if (fread(size_bytes, 1, 2, file.get()) != 2) break;
        const uint16_t segment_size = static_cast<uint16_t>((size_bytes[0] << 8) | size_bytes[1]);
        if (segment_size < 2) break;

        const size_t payload_size = segment_size - 2;
        if (marker == 0xE1) {
            std::vector<uint8_t> payload(payload_size);
            if (fread(payload.data(), 1, payload_size, file.get()) != payload_size) break;

            int orientation = 1;
            if (parse_exif_app1(payload.data(), payload.size(), orientation)) return orientation;
            continue;
        }

        if (fseek(file.get(), static_cast<long>(payload_size), SEEK_CUR) != 0) break;
    }

    return 1;
}

static int read_exif_orientation(const uint8_t* bytes, size_t size) {
    if (!bytes || size < 4 || bytes[0] != 0xFF || bytes[1] != 0xD8) return 1;

    size_t offset = 2;
    while (offset < size) {
        if (bytes[offset++] != 0xFF) break;
        while (offset < size && bytes[offset] == 0xFF) ++offset;
        if (offset >= size) break;

        const uint8_t marker = bytes[offset++];
        if (marker == 0x00) continue;
        if (marker == 0xD9 || marker == 0xDA) break;
        if (marker_has_no_payload(marker)) continue;
        if (size - offset < 2) break;

        const uint16_t segment_size =
            static_cast<uint16_t>((bytes[offset] << 8) | bytes[offset + 1]);
        offset += 2;
        if (segment_size < 2) break;

        const size_t payload_size = segment_size - 2;
        if (payload_size > size - offset) break;

        if (marker == 0xE1) {
            int orientation = 1;
            if (parse_exif_app1(bytes + offset, payload_size, orientation)) return orientation;
        }
        offset += payload_size;
    }

    return 1;
}

// ---------------------------------------------------------------------------
// Pixel-level transformations
// ---------------------------------------------------------------------------

/**
 * @brief Returns whether an EXIF orientation swaps width and height.
 */
static bool orientation_swaps_dimensions(int orientation) {
    return orientation >= 5 && orientation <= 8;
}

/**
 * Applies all eight EXIF orientation values. Source and destination must not overlap.
 */
static bool apply_orientation(const unsigned char* source, unsigned char* destination, int width,
                              int height, int channels, int orientation) {
    if (!source || !destination) return false;
    if (width <= 0 || height <= 0 || channels <= 0 || orientation < 1 || orientation > 8) {
        return false;
    }

    const size_t output_width =
        static_cast<size_t>(orientation_swaps_dimensions(orientation) ? height : width);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int destination_x = x;
            int destination_y = y;

            switch (orientation) {
                case 2:  // Mirror horizontally.
                    destination_x = width - 1 - x;
                    break;
                case 3:  // Rotate 180 degrees.
                    destination_x = width - 1 - x;
                    destination_y = height - 1 - y;
                    break;
                case 4:  // Mirror vertically.
                    destination_y = height - 1 - y;
                    break;
                case 5:  // Transpose across the top-left/bottom-right diagonal.
                    destination_x = y;
                    destination_y = x;
                    break;
                case 6:  // Rotate 90 degrees clockwise.
                    destination_x = height - 1 - y;
                    destination_y = x;
                    break;
                case 7:  // Transpose across the top-right/bottom-left diagonal.
                    destination_x = height - 1 - y;
                    destination_y = width - 1 - x;
                    break;
                case 8:  // Rotate 270 degrees clockwise.
                    destination_x = y;
                    destination_y = width - 1 - x;
                    break;
                default:
                    break;
            }

            const size_t source_index =
                (static_cast<size_t>(y) * width + x) * static_cast<size_t>(channels);
            const size_t destination_index =
                (static_cast<size_t>(destination_y) * output_width + destination_x) *
                static_cast<size_t>(channels);
            std::memcpy(destination + destination_index, source + source_index,
                        static_cast<size_t>(channels));
        }
    }

    return true;
}

/**
 * @brief Scales down @p original dimensions to fit inside (@p max_w, @p max_h)
 *        while preserving aspect ratio. Never upscales.
 */
static void fit_inside_box(int original_w, int original_h, int max_w, int max_h, int& out_w,
                           int& out_h) {
    if (original_w <= 0 || original_h <= 0 || max_w <= 0 || max_h <= 0) {
        out_w = 0;
        out_h = 0;
        return;
    }

    const double ratio_w = static_cast<double>(max_w) / original_w;
    const double ratio_h = static_cast<double>(max_h) / original_h;
    const double ratio = std::min(ratio_w, ratio_h);

    if (ratio >= 1.0) {
        out_w = original_w;
        out_h = original_h;
    } else {
        out_w = std::max(1, static_cast<int>(original_w * ratio));
        out_h = std::max(1, static_cast<int>(original_h * ratio));
    }
}

// ---------------------------------------------------------------------------
// Shared encode-to-base64 finalisation
// Avoids duplicating the malloc / memcpy / null-check block in both APIs.
// ---------------------------------------------------------------------------

static char* encode_jpeg_to_base64_cstr(const std::vector<unsigned char>& jpeg_buffer) {
    if (jpeg_buffer.empty()) return nullptr;

    std::string base64 = base64_encode(jpeg_buffer.data(), jpeg_buffer.size());
    char* result = static_cast<char*>(malloc(base64.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, base64.c_str(), base64.size());
    result[base64.size()] = '\0';
    return result;
}

struct StbiImageDeleter {
    void operator()(unsigned char* image) const { stbi_image_free(image); }
};

using StbiImage = std::unique_ptr<unsigned char, StbiImageDeleter>;

static int clamp_quality(int quality) { return std::max(1, std::min(quality, 100)); }

static char* compress_decoded_image(StbiImage image, int width, int height, int channels,
                                    int orientation, int quality, int max_width, int max_height) {
    if (!image || width <= 0 || height <= 0 || channels < 1 || channels > 4) return nullptr;

    size_t image_size = 0;
    if (!calculate_buffer_size(width, height, channels, image_size)) return nullptr;

    const unsigned char* pixels = image.get();
    int current_width = width;
    int current_height = height;
    std::vector<unsigned char> oriented_buffer;

    if (orientation >= 2 && orientation <= 8) {
        oriented_buffer.resize(image_size);
        if (!apply_orientation(image.get(), oriented_buffer.data(), width, height, channels,
                               orientation)) {
            return nullptr;
        }

        pixels = oriented_buffer.data();
        if (orientation_swaps_dimensions(orientation)) {
            std::swap(current_width, current_height);
        }
        image.reset();
    }

    const int width_limit = max_width > 0 ? max_width : current_width;
    const int height_limit = max_height > 0 ? max_height : current_height;
    int output_width = current_width;
    int output_height = current_height;
    fit_inside_box(current_width, current_height, width_limit, height_limit, output_width,
                   output_height);
    if (output_width <= 0 || output_height <= 0) return nullptr;

    std::vector<unsigned char> resized_buffer;
    if (output_width != current_width || output_height != current_height) {
        if (!resize_image(pixels, current_width, current_height, channels, resized_buffer,
                          output_width, output_height)) {
            return nullptr;
        }

        pixels = resized_buffer.data();
        image.reset();
        std::vector<unsigned char>().swap(oriented_buffer);
    }

    std::vector<unsigned char> jpeg_buffer;
    if (!compress_to_jpeg_buffer(pixels, output_width, output_height, channels,
                                 clamp_quality(quality), jpeg_buffer)) {
        return nullptr;
    }

    return encode_jpeg_to_base64_cstr(jpeg_buffer);
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
        if (!path || path[0] == '\0') return nullptr;

        const int orientation = read_exif_orientation(path);
        int width = 0;
        int height = 0;
        int channels = 0;
        StbiImage image(stbi_load(path, &width, &height, &channels, 0));
        if (!image) return nullptr;

        return compress_decoded_image(std::move(image), width, height, channels, orientation,
                                      quality, max_width, max_height);
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
 * @brief Loads, EXIF-corrects, resizes and JPEG-compresses an encoded image buffer.
 *
 * @param input_bytes Pointer to the encoded input image.
 * @param input_size  Size of the input buffer in bytes.
 * @param quality     JPEG quality (1–100; clamped automatically).
 * @param max_width   Maximum output width. Pass 0 for no width constraint.
 * @param max_height  Maximum output height. Pass 0 for no height constraint.
 * @return Heap-allocated Base64 C string, or nullptr on failure.
 *         Caller must free it with image_compressor_free_string().
 */
extern "C" char* image_compressor_from_bytes(const uint8_t* input_bytes, int input_size,
                                             int quality, int max_width, int max_height) {
    try {
        if (!input_bytes || input_size <= 0) return nullptr;

        const int orientation = read_exif_orientation(input_bytes, static_cast<size_t>(input_size));
        int width = 0;
        int height = 0;
        int channels = 0;
        StbiImage image(
            stbi_load_from_memory(input_bytes, input_size, &width, &height, &channels, 0));
        if (!image) return nullptr;

        return compress_decoded_image(std::move(image), width, height, channels, orientation,
                                      quality, max_width, max_height);
    } catch (...) {
        return nullptr;
    }
}
