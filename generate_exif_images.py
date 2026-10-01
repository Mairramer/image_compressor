import struct
from PIL import Image

img = Image.new('RGB', (2, 4), color='red')

# We need to construct a minimal EXIF tag for orientation.
# Pillow provides an exif parameter.
exif = img.getexif()
# Tag 274 is Orientation.
exif[274] = 6 # Rotate 90 CW
img.save('src/tests/test_assets/exif_6.jpg', exif=exif)

exif[274] = 3 # Rotate 180
img.save('src/tests/test_assets/exif_3.jpg', exif=exif)

print("Generated.")
