#ifndef IMAGE_COMPRESSOR_H
#define IMAGE_COMPRESSOR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compress an image file located at `path` into a JPEG Base64 string.
 * 
 * @param path       File path to the image.
 * @param quality    JPEG quality (1-100).
 * @param max_size   Maximum width or height for resizing. Use 0 for no resize.
 * @return           Pointer to a null-terminated Base64 string allocated with malloc.
 *                   Must be freed by calling `free_compressed_image`.
 *                   Returns nullptr on failure.
 */
char* compress_image(const char* path, int quality, int max_size);

/**
 * Frees the pointer returned by `compress_image`.
 */
void free_compressed_image(char* ptr);

#ifdef __cplusplus
}
#endif

#endif // IMAGE_COMPRESSOR_H
