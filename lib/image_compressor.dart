import 'dart:async';
import 'dart:ffi' as ffi;
import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

// final DynamicLibrary _dylib = () {
//   if (Platform.isMacOS || Platform.isIOS) {
//     return DynamicLibrary.open('libimage_compressor.dylib');
//   } else if (Platform.isAndroid || Platform.isLinux) {
//     return DynamicLibrary.open('libimage_compressor.so');
//   } else if (Platform.isWindows) {
//     return DynamicLibrary.open('image_compressor.dll');
//   } else {
//     throw UnsupportedError('Unsupported platform');
//   }
// }();

// final ImageCompressorBindings _bindings = ImageCompressorBindings(_dylib);

// class _CompressRequest {
//   final SendPort replyPort;
//   final String path;
//   final int quality;
//   final int maxSize;

//   _CompressRequest(this.replyPort, this.path, this.quality, this.maxSize);
// }

// void _compressIsolateEntry(_CompressRequest request) {
//   try {
//     final Pointer<Utf8> pathUtf8 = request.path.toNativeUtf8();
//     final Pointer<ffi.Char> pathChar = pathUtf8.cast<ffi.Char>();
//     final resultPtr = _bindings.compress_image(pathChar, request.quality, request.maxSize);
//     calloc.free(pathUtf8);

//     if (resultPtr == nullptr) {
//       request.replyPort.send(Exception('Compressão falhou'));
//       return;
//     }

//     final compressed = resultPtr.cast<Utf8>().toDartString();
//     _bindings.free_compressed_image(resultPtr);

//     request.replyPort.send(compressed);
//   } catch (e) {
//     request.replyPort.send(e);
//   }
// }

// /// Compressão de imagem com isolate interno para não travar UI.
// /// [quality] entre 1 e 100 (padrão 75).
// /// [maxSize] máxima largura ou altura para redimensionar (padrão 1080).
// Future<String> compressImage(String path, {int quality = 75, int maxSize = 1080}) async {
//   final receivePort = ReceivePort();
//   final request = _CompressRequest(receivePort.sendPort, path, quality, maxSize);
//   await Isolate.spawn(_compressIsolateEntry, request);
//   final result = await receivePort.first;
//   receivePort.close();

//   if (result is Exception) {
//     throw result;
//   } else if (result is String) {
//     return result;
//   } else {
//     throw Exception('Erro desconhecido na compressão');
//   }
// }

final DynamicLibrary lib = Platform.isAndroid
    ? DynamicLibrary.open('libimage_compressor.so')
    : Platform.isIOS
    ? DynamicLibrary.process()
    : throw UnsupportedError('Unsupported platform');

typedef CompressImageC = Pointer<Utf8> Function(Pointer<Utf8> path, Int32 quality, Int32 maxSize);
typedef CompressImageD = Pointer<Utf8> Function(Pointer<Utf8> path, int quality, int maxSize);

typedef FreeStringC = Void Function(Pointer<Utf8>);
typedef FreeStringD = void Function(Pointer<Utf8>);

final CompressImageD compressImage = lib.lookupFunction<CompressImageC, CompressImageD>('compress_image');
final FreeStringD freeString = lib.lookupFunction<FreeStringC, FreeStringD>('free_compressed_image');

Future<String> compressImageFromP(String path, {int quality = 70, int maxSize = 1080}) async {
  final pathPtr = path.toNativeUtf8();
  try {
    final resultPtr = compressImage(pathPtr, quality, maxSize);
    if (resultPtr == nullptr) {
      throw Exception('Compressão falhou');
    }
    final result = resultPtr.toDartString();
    freeString(resultPtr);
    return result;
  } finally {
    calloc.free(pathPtr);
  }
}
