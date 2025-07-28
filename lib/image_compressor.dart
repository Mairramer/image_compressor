import 'dart:async';
import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

/// Interface that defines the contract for image compression implementations.
abstract class ImageCompressor {
  /// Compresses an image from the given [path].
  ///
  /// [quality] specifies the compression quality (default 75).
  /// [maxWidth] specifies the maximum width allowed (default 1080).
  /// [maxHeight] specifies the maximum height allowed (default 1920).
  ///
  /// Defaults are set for **portrait mode** images.
  ///
  /// Returns a [Future] that completes with the compressed image encoded as a Base64 string.
  Future<String> compressImageFromPath(
    String path, {
    int quality = 75,
    int maxWidth = 1080,
    int maxHeight = 1920,
  });
}

/// Native implementation of [ImageCompressor] using Dart FFI.
///
/// This class loads the native library and binds to the native functions
/// to perform image compression in native code.
class NativeImageCompressor implements ImageCompressor {
  late final DynamicLibrary _nativeLib;

  // Native function pointers
  late final Pointer<Utf8> Function(Pointer<Utf8>, int, int, int) _fromPath;
  late final void Function(Pointer<Utf8>) _freeString;

  /// Constructs a [NativeImageCompressor] and loads the native library.
  NativeImageCompressor() {
    _nativeLib = _loadLibrary();

    _fromPath = _nativeLib
        .lookupFunction<
          Pointer<Utf8> Function(Pointer<Utf8>, Int32, Int32, Int32),
          Pointer<Utf8> Function(Pointer<Utf8>, int, int, int)
        >('image_compressor_from_path');

    _freeString = _nativeLib.lookupFunction<Void Function(Pointer<Utf8>), void Function(Pointer<Utf8>)>(
      'image_compressor_free_string',
    );
  }

  /// Loads the dynamic library according to the current platform.
  ///
  /// For Android, opens the shared object `.so` file.
  /// For iOS, uses the process image since the library is embedded.
  /// Throws [UnsupportedError] if the platform is not supported.
  DynamicLibrary _loadLibrary() {
    if (Platform.isAndroid) {
      return DynamicLibrary.open('libimage_compressor.so');
    } else if (Platform.isIOS) {
      return DynamicLibrary.process();
    } else {
      throw UnsupportedError('Platform ${Platform.operatingSystem} is not supported.');
    }
  }

  @override
  Future<String> compressImageFromPath(
    String path, {
    int quality = 75,
    int maxWidth = 1080,
    int maxHeight = 1920,
  }) async {
    if (path.isEmpty) {
      throw ArgumentError('Image path cannot be empty.');
    }
    final Pointer<Utf8> pathPtr = path.toNativeUtf8();
    try {
      final Pointer<Utf8> resultPtr = _fromPath(pathPtr, quality, maxWidth, maxHeight);

      if (resultPtr == nullptr) {
        throw Exception('Image compression failed: native returned null pointer.');
      }

      final String result = resultPtr.toDartString();
      _freeString(resultPtr);
      return result;
    } finally {
      calloc.free(pathPtr);
    }
  }
}
