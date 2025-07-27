import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:image/image.dart' as img_pkg;
import 'package:image_compressor/image_compressor.dart' as image_compressor;
import 'package:image_picker/image_picker.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(home: HomePage());
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  File? _imageFile;
  Uint8List? _dartOutput;
  Uint8List? _ffiOutput;
  String _result = '';

  Future<void> _pickImage() async {
    final picker = ImagePicker();
    final picked = await picker.pickImage(source: ImageSource.gallery);
    if (picked != null) {
      setState(() {
        _imageFile = File(picked.path);
        _dartOutput = null;
        _ffiOutput = null;
        _result = '';
      });
    }
  }

  Future<({Duration duration, Uint8List output})> _runBenchmark(
    Future<String> Function(String path) compressor,
    String path,
  ) async {
    final start = DateTime.now();
    final resultBase64 = await compressor(path);
    final output = base64Decode(resultBase64);
    final duration = DateTime.now().difference(start);
    return (duration: duration, output: output);
  }

  Future<({Duration duration, Uint8List output})> _runBenchmarkFFI(
    Future<String> Function(String path) compressor,
    String path,
  ) async {
    final start = DateTime.now();
    final output = await compressor(path);
    final duration = DateTime.now().difference(start);
    return (duration: duration, output: base64Decode(output));
  }

  double _getPhotoSize(String? base64String) {
    if (base64String == null) {
      return 0;
    }

    final List<int> bytes = base64Decode(base64String);

    final double kilobytes = (bytes.length / 1024);

    return double.parse(kilobytes.toStringAsFixed(2));
  }

  Future<void> _benchmarkBoth() async {
    if (_imageFile == null) return;
    final path = _imageFile!.path;
    final originalSize = await _imageFile!.length();

    final dart = await _runBenchmark(AppImageCompression.compressImage, path);
    final ffi = await _runBenchmarkFFI((p) => image_compressor.compressImageFromP(p), path);

    setState(() {
      _dartOutput = dart.output;
      _ffiOutput = ffi.output;
      final originalSizeKB = (originalSize / 1024).toStringAsFixed(2);
      final dartSizeKB = (dart.output.lengthInBytes / 1024).toStringAsFixed(2);
      final ffiSizeKB = (ffi.output.lengthInBytes / 1024).toStringAsFixed(2);

      _result =
          '''
🏁 **Benchmark de Compressão**

📷 Original: $originalSize bytes

🧪 Dart:
  • Tempo: ${dart.duration.inMilliseconds} ms
  • Tamanho: ${dart.output.lengthInBytes} bytes

⚙️ FFI:
  • Tempo: ${ffi.duration.inMilliseconds} ms
  • Tamanho: ${ffi.output.lengthInBytes} bytes
''';
      if (_dartOutput != null) {
        _result += '\n\n📊 Tamanho da Imagem Dart: $dartSizeKB KB';
      }
      if (_ffiOutput != null) {
        _result += '\n📊 Tamanho da Imagem FFI: $ffiSizeKB KB';
      }
      // _result += '\n\n📏 Tamanho Original: ${_getPhotoSize(base64Encode(_imageFile!.readAsBytesSync()))} KB';
    });

    print(_result);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Benchmark de Compressão')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: SingleChildScrollView(
          child: Column(
            children: [
              ElevatedButton(onPressed: _pickImage, child: const Text('Selecionar Imagem')),
              if (_imageFile != null) ...[
                Image.file(_imageFile!, height: 150),
                const SizedBox(height: 16),
                ElevatedButton(onPressed: _benchmarkBoth, child: const Text('Rodar Benchmark')),
              ],
              const SizedBox(height: 16),
              if (_result.isNotEmpty) Text(_result, style: const TextStyle(fontFamily: 'monospace')),
              if (_dartOutput != null)
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [const Text('Imagem Dart'), Image.memory(_dartOutput!, height: 150)],
                ),
              if (_ffiOutput != null)
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [const Text('Imagem FFI'), Image.memory(_ffiOutput!, height: 150)],
                ),
            ],
          ),
        ),
      ),
    );
  }
}

Future<Uint8List> _readFileBytes(String path) async {
  try {
    final file = File(path);
    if (!await file.exists()) {
      throw Exception('File not found: $path');
    }
    return await file.readAsBytes();
  } catch (e) {
    debugPrint('Error reading file: $e');
    rethrow;
  }
}

mixin AppImageCompression {
  static Future<String> compressImage(String path) async {
    final bytes = await compute(_readFileBytes, path);

    return await compute((List<dynamic> args) => compressImageFromBase64WithQuality(args[0]), [base64Encode(bytes)]);
  }

  static String compressImageFromBase64WithQuality(
    String? imageBase64, {
    int maxWidth = 1080,
    int maxHeight = 1920,
    int quality = 70,
  }) {
    if (imageBase64 == null || imageBase64.isEmpty) {
      return '';
    }

    final Uint8List byteImage = base64Decode(imageBase64);

    final img_pkg.Image? img = img_pkg.decodeImage(byteImage);
    if (img == null) {
      return '';
    }

    int newWidth = img.width;
    int newHeight = img.height;

    if (img.width > maxWidth || img.height > maxHeight) {
      final widthRatio = maxWidth / img.width;
      final heightRatio = maxHeight / img.height;
      final resizeRatio = widthRatio < heightRatio ? widthRatio : heightRatio;

      newWidth = (img.width * resizeRatio).round();
      newHeight = (img.height * resizeRatio).round();
    }

    img_pkg.Image resizedImage = img;
    if (newWidth != img.width || newHeight != img.height) {
      resizedImage = img_pkg.copyResize(img, width: newWidth, height: newHeight);
    }

    final resizedData = img_pkg.encodeJpg(resizedImage, quality: quality);

    return base64Encode(resizedData);
  }
}
