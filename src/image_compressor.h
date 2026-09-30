#ifndef IMAGE_COMPRESSOR_H
#define IMAGE_COMPRESSOR_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compresses an encoded image buffer into a JPEG Base64 string.
 * EXIF orientation is applied when present.
 *
 * @param input_bytes Pointer to the encoded input image.
 * @param input_size  Size of the input buffer in bytes.
 * @param quality     JPEG quality (1-100).
 * @param max_width   Maximum width (0 to keep original size).
 * @param max_height  Maximum height (0 to keep original size).
 * @return Base64 string allocated with malloc. Must be freed using
 *         image_compressor_free_string.
 */
__attribute__((visibility("default"))) char* image_compressor_from_bytes(const uint8_t* input_bytes,
                                                                         int input_size,
                                                                         int quality, int max_width,
                                                                         int max_height);

/**
 * Compress an image from a file path into a JPEG Base64 string.
 *
 * @param path       File path to the image.
 * @param quality    JPEG quality (1-100).
 * @param max_width  Maximum width for resizing. Use 0 for no width limit.
 * @param max_height Maximum height for resizing. Use 0 for no height limit.
 * @return Pointer to a null-terminated Base64 string allocated with malloc.
 *         Must be freed by calling `image_compressor_free_string`.
 *         Returns nullptr on failure.
 */
__attribute__((visibility("default"))) char* image_compressor_from_path(const char* path,
                                                                        int quality, int max_width,
                                                                        int max_height);

/**
 * Frees any C string returned by `image_compressor_from_path` or
 * `image_compressor_from_bytes`.
 */
__attribute__((visibility("default"))) void image_compressor_free_string(char* ptr);

/**
 * Processes an image byte array (resize, rotate, mirror, compress) and returns the new JPEG bytes.
 *
 * @param input_bytes      Pointer to the encoded input image.
 * @param input_size       Size of the input buffer in bytes.
 * @param rotation_degrees Rotation in degrees (0, 90, 180, 270).
 * @param mirror           True to mirror horizontally.
 * @param max_dimension    Maximum width/height.
 * @param quality          JPEG quality (1-100).
 * @param out_size         Pointer to an integer where the output size will be written.
 * @return Pointer to a newly allocated byte array containing the JPEG data, or nullptr if no processing was needed or on failure.
 *         If nullptr is returned and *out_size is 0, it means no processing was needed.
 *         Must be freed by calling `image_compressor_free_buffer`.
 */
__attribute__((visibility("default"))) uint8_t* image_compressor_process_image(
    const uint8_t* input_bytes, int input_size, int rotation_degrees, bool mirror,
    int max_dimension, int quality, int* out_size);

/**
 * Frees any buffer returned by `image_compressor_process_image`.
 */
__attribute__((visibility("default"))) void image_compressor_free_buffer(uint8_t* ptr);

#ifdef __cplusplus
}
#endif

#endif  // IMAGE_COMPRESSOR_H
