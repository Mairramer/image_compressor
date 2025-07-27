import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:image/image.dart' as img_pkg;
import 'package:image_compressor/image_compressor.dart'; // import interface and implementation
import 'package:image_picker/image_picker.dart';

void main() => runApp(const MyApp());

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) => const MaterialApp(home: HomePage());
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
  bool _isProcessing = false;

  final ImagePicker _picker = ImagePicker();

  late final ImageCompressor _compressor;

  @override
  void initState() {
    super.initState();
    _compressor = NativeImageCompressor();
  }

  Future<void> _pickImage() async {
    try {
      final picked = await _picker.pickImage(source: ImageSource.gallery);
      if (picked != null) {
        setState(() {
          _imageFile = File(picked.path);
          _dartOutput = null;
          _ffiOutput = null;
          _result = '';
        });
      }
    } catch (e) {
      _showError('Error selecting image: $e');
    }
  }

  Future<_BenchmarkResult> _runBenchmark({
    required Future<String> Function(String path) compressor,
    required String path,
  }) async {
    final stopwatch = Stopwatch()..start();
    final base64Result = await compressor(path);
    stopwatch.stop();
    final outputBytes = base64Decode(base64Result);
    return _BenchmarkResult(duration: stopwatch.elapsed, output: outputBytes);
  }

  Future<void> _benchmarkBoth() async {
    if (_imageFile == null) return;

    setState(() {
      _isProcessing = true;
      _result = '';
    });

    try {
      final originalSize = await _imageFile!.length();

      final dartResult = await _runBenchmark(compressor: AppImageCompression.compressImage, path: _imageFile!.path);

      final ffiResult = await _runBenchmark(compressor: _compressor.compressImageFromPath, path: _imageFile!.path);

      setState(() {
        _dartOutput = dartResult.output;
        _ffiOutput = ffiResult.output;

        _result =
            '''
🏁 Compression Benchmark

📷 Original:
  • Size: ${_formatBytes(originalSize)}

🧪 Dart:
  • Time: ${dartResult.duration.inMilliseconds} ms
  • Size: ${_formatBytes(dartResult.output.length)}

⚙️ FFI:
  • Time: ${ffiResult.duration.inMilliseconds} ms
  • Size: ${_formatBytes(ffiResult.output.length)}
''';
      });
      print(_result);
    } catch (e) {
      _showError('Error during benchmark: $e');
    } finally {
      setState(() => _isProcessing = false);
    }
  }

  String _formatBytes(int bytes, [int decimals = 2]) {
    if (bytes <= 0) return "0 B";
    const suffixes = ['B', 'KB', 'MB', 'GB'];
    final i = (bytes > 0) ? (math.log(bytes) / math.log(1024)).floor() : 0;
    final size = bytes / math.pow(1024, i);
    return '${size.toStringAsFixed(decimals)} ${suffixes[i]}';
  }

  void _showError(String message) {
    if (context.mounted) {
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(message)));
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Compression Benchmark')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: SingleChildScrollView(
          child: Column(
            children: [
              ElevatedButton.icon(
                onPressed: _isProcessing ? null : _pickImage,
                icon: const Icon(Icons.photo_library),
                label: const Text('Select Image'),
              ),
              if (_imageFile != null) ...[
                const SizedBox(height: 16),
                Image.file(_imageFile!, height: 150),
                const SizedBox(height: 16),
                ElevatedButton.icon(
                  onPressed: _isProcessing ? null : _benchmarkBoth,
                  icon: _isProcessing
                      ? const SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2))
                      : const Icon(Icons.timer),
                  label: Text(_isProcessing ? 'Processing...' : 'Run Benchmark'),
                ),
              ],
              const SizedBox(height: 24),
              if (_result.isNotEmpty)
                SelectableText(_result, style: const TextStyle(fontFamily: 'monospace', fontSize: 14)),
              const SizedBox(height: 24),
              if (_dartOutput != null) ...[
                const Text('Compressed Image - Dart', style: TextStyle(fontWeight: FontWeight.bold)),
                const SizedBox(height: 8),
                Image.memory(_dartOutput!, height: 150),
              ],
              if (_ffiOutput != null) ...[
                const SizedBox(height: 16),
                const Text('Compressed Image - FFI', style: TextStyle(fontWeight: FontWeight.bold)),
                const SizedBox(height: 8),
                Image.memory(_ffiOutput!, height: 150),
              ],
            ],
          ),
        ),
      ),
    );
  }
}

class _BenchmarkResult {
  final Duration duration;
  final Uint8List output;

  _BenchmarkResult({required this.duration, required this.output});
}

Future<Uint8List> _readFileBytes(String path) async {
  final file = File(path);
  if (!await file.exists()) {
    throw Exception('File not found: $path');
  }
  return await file.readAsBytes();
}

mixin AppImageCompression {
  static Future<String> compressImage(String path) async {
    final bytes = await compute(_readFileBytes, path);
    return await compute((List<dynamic> args) => compressImageFromBase64WithQuality(args[0]), [base64Encode(bytes)]);
  }

  static String compressImageFromBase64WithQuality(
    String imageBase64, {
    int maxWidth = 1080,
    int maxHeight = 1920,
    int quality = 70,
  }) {
    if (imageBase64.isEmpty) return '';

    final byteImage = base64Decode(imageBase64);
    final img_pkg.Image? img = img_pkg.decodeImage(byteImage);
    if (img == null) return '';

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
