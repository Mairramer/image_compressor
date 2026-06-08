import 'dart:async';
import 'dart:ffi';
import 'dart:io';
import 'dart:typed_data';

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

  /// Compresses an image from the given [bytes].
  ///
  /// [quality] specifies the compression quality (default 75).
  /// [maxWidth] specifies the maximum width allowed (default 1080).
  /// [maxHeight] specifies the maximum height allowed (default 1920).
  ///
  /// Returns a [Future] that completes with the compressed image encoded as a Base64 string.
  Future<String> compressImageFromBytes(
    Uint8List bytes, {
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
  late final Pointer<Utf8> Function(Pointer<Uint8>, int, int, int, int) _fromBytes;
  late final void Function(Pointer<Utf8>) _freeString;

  /// Constructs a [NativeImageCompressor] and loads the native library.
  NativeImageCompressor() {
    _nativeLib = _loadLibrary();

    _fromPath = _nativeLib
        .lookupFunction<
          Pointer<Utf8> Function(Pointer<Utf8>, Int32, Int32, Int32),
          Pointer<Utf8> Function(Pointer<Utf8>, int, int, int)
        >('image_compressor_from_path');

    _fromBytes = _nativeLib
        .lookupFunction<
          Pointer<Utf8> Function(Pointer<Uint8>, Int32, Int32, Int32, Int32),
          Pointer<Utf8> Function(Pointer<Uint8>, int, int, int, int)
        >('image_compressor_from_bytes');

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

    final file = File(path);
    if (!file.existsSync()) {
      throw ArgumentError('Image file does not exist at path: $path');
    }

    final Pointer<Utf8> pathPtr = path.toNativeUtf8();
    try {
      final Pointer<Utf8> resultPtr = _fromPath(pathPtr, quality, maxWidth, maxHeight);
      return _processNativeResult(resultPtr, context: 'compressImageFromPath(path: $path)');
    } finally {
      calloc.free(pathPtr);
    }
  }

  @override
  Future<String> compressImageFromBytes(
    Uint8List bytes, {
    int quality = 75,
    int maxWidth = 1080,
    int maxHeight = 1920,
  }) async {
    if (bytes.isEmpty) {
      throw ArgumentError('Image bytes cannot be empty.');
    }

    final Pointer<Uint8> bytesPtr = malloc.allocate<Uint8>(bytes.length);
    try {
      bytesPtr.asTypedList(bytes.length).setAll(0, bytes);

      final Pointer<Utf8> resultPtr = _fromBytes(
        bytesPtr,
        bytes.length,
        quality,
        maxWidth,
        maxHeight,
      );

      return _processNativeResult(resultPtr, context: 'compressImageFromBytes(length: ${bytes.length})');
    } finally {
      malloc.free(bytesPtr);
    }
  }

  /// Processes the result from native code, converts it to a Dart string,
  /// and frees the native memory.
  ///
  /// [context] is an optional description included in the error message
  /// to help identify which call failed.
  String _processNativeResult(Pointer<Utf8> resultPtr, {String? context}) {
    if (resultPtr == nullptr) {
      final where = context != null ? ' [$context]' : '';
      throw Exception(
        'Image compression failed: native returned null pointer.$where '
        'Possible causes: file not found, unsupported image format, '
        'out-of-memory, or resize/JPEG encoding failure.',
      );
    }

    try {
      final String result = resultPtr.toDartString();
      if (result.isEmpty) {
        final where = context != null ? ' [$context]' : '';
        throw Exception('Image compression failed: native returned an empty string.$where');
      }
      return result;
    } finally {
      _freeString(resultPtr);
    }
  }
}
