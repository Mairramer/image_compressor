#ifndef IMAGE_COMPRESSOR_H
#define IMAGE_COMPRESSOR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compress an image from a file path into a JPEG Base64 string.
 * 
 * @param path       File path to the image.
 * @param quality    JPEG quality (1-100).
 * @param max_size   Maximum width or height for resizing. Use 0 for no resize.
 * @return           Pointer to a null-terminated Base64 string allocated with malloc.
 *                   Must be freed by calling `image_compressor_free_string`.
 *                   Returns nullptr on failure.
 */
__attribute__((visibility("default")))
char* image_compressor_from_path(const char* path, int quality, int max_size);

/**
 * Frees any C string returned by `image_compressor_from_path`.
 */
__attribute__((visibility("default")))
void image_compressor_free_string(char* ptr);

#ifdef __cplusplus
}
#endif

#endif // IMAGE_COMPRESSOR_H
