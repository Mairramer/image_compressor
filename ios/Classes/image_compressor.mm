// Relative import to be able to reuse the C sources.
// See the comment in ../image_compressor.podspec for more information.
#include "../../src/image_compressor.h"

extern "C" {
char *image_compressor_from_path(const char *path, int quality, int max_size);
void image_compressor_free_string(char *ptr);
}