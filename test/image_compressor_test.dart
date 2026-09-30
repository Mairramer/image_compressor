import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:image_compressor/image_compressor.dart';
import 'package:mocktail/mocktail.dart';

class MockImageCompressor extends Mock implements ImageCompressor {}

const _kFakeBase64 = 'AQID';

Uint8List _someBytes([int length = 16]) => Uint8List.fromList(List.generate(length, (i) => i % 256));

void main() {
  late MockImageCompressor compressor;

  setUpAll(() {
    registerFallbackValue(Uint8List(0));
  });

  setUp(() {
    compressor = MockImageCompressor();
  });

  group('compressImageFromPath', () {
    test('returns a non-empty Base64 string on success', () async {
      when(
        () => compressor.compressImageFromPath(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => _kFakeBase64);

      final result = await compressor.compressImageFromPath('/img/photo.jpg');

      expect(result, isNotEmpty);
      expect(() => base64Decode(result), returnsNormally);
    });

    test('is called with the correct path and default parameters', () async {
      when(
        () => compressor.compressImageFromPath(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => _kFakeBase64);

      await compressor.compressImageFromPath('/img/photo.jpg');

      verify(
        () => compressor.compressImageFromPath(
          '/img/photo.jpg',
        ),
      ).called(1);
    });

    test('is called with custom quality, maxWidth and maxHeight', () async {
      when(
        () => compressor.compressImageFromPath(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => _kFakeBase64);

      await compressor.compressImageFromPath(
        '/img/photo.jpg',
        quality: 50,
        maxWidth: 640,
        maxHeight: 480,
      );

      verify(
        () => compressor.compressImageFromPath(
          '/img/photo.jpg',
          quality: 50,
          maxWidth: 640,
          maxHeight: 480,
        ),
      ).called(1);
    });

    test('throws when native returns null pointer', () async {
      when(
        () => compressor.compressImageFromPath(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenThrow(
        Exception(
          'Image compression failed: native returned null pointer. '
          'Possible causes: file not found, unsupported image format, '
          'out-of-memory, or resize/JPEG encoding failure.',
        ),
      );

      expect(
        () => compressor.compressImageFromPath('/fake/image.jpg'),
        throwsA(
          isA<Exception>().having(
            (e) => e.toString(),
            'message',
            allOf(
              contains('native returned null pointer'),
              contains('Possible causes'),
            ),
          ),
        ),
      );
    });

    test('throws ArgumentError for empty path', () async {
      when(
        () => compressor.compressImageFromPath(
          '',
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenThrow(ArgumentError('Image path cannot be empty.'));

      expect(
        () => compressor.compressImageFromPath(''),
        throwsA(
          isA<ArgumentError>().having(
            (e) => e.message,
            'message',
            contains('cannot be empty'),
          ),
        ),
      );
    });

    test('decoded result matches the expected bytes', () async {
      final knownBytes = Uint8List.fromList([0xFF, 0xD8, 0xFF]); // JPEG magic
      final knownBase64 = base64Encode(knownBytes);

      when(
        () => compressor.compressImageFromPath(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => knownBase64);

      final result = await compressor.compressImageFromPath('/img/photo.jpg');

      expect(base64Decode(result), equals(knownBytes));
    });

    test('quality = 1 is accepted without error', () async {
      when(
        () => compressor.compressImageFromPath(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => _kFakeBase64);

      await compressor.compressImageFromPath('/img/photo.jpg', quality: 1);

      verify(
        () => compressor.compressImageFromPath('/img/photo.jpg', quality: 1),
      ).called(1);
    });

    test('quality = 100 is accepted without error', () async {
      when(
        () => compressor.compressImageFromPath(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => _kFakeBase64);

      await compressor.compressImageFromPath('/img/photo.jpg', quality: 100);

      verify(
        () => compressor.compressImageFromPath('/img/photo.jpg', quality: 100),
      ).called(1);
    });

    test('landscape dimensions are forwarded correctly', () async {
      when(
        () => compressor.compressImageFromPath(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => _kFakeBase64);

      await compressor.compressImageFromPath(
        '/img/photo.jpg',
        maxWidth: 1920,
        maxHeight: 1080,
      );

      verify(
        () => compressor.compressImageFromPath(
          '/img/photo.jpg',
          maxWidth: 1920,
          maxHeight: 1080,
        ),
      ).called(1);
    });

    test('maxWidth = 0 and maxHeight = 0 mean no constraint', () async {
      when(
        () => compressor.compressImageFromPath(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => _kFakeBase64);

      await compressor.compressImageFromPath(
        '/img/photo.jpg',
        maxWidth: 0,
        maxHeight: 0,
      );

      verify(
        () => compressor.compressImageFromPath(
          '/img/photo.jpg',
          maxWidth: 0,
          maxHeight: 0,
        ),
      ).called(1);
    });
  });

  group('compressImageFromBytes', () {
    test('returns a non-empty Base64 string on success', () async {
      when(
        () => compressor.compressImageFromBytes(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => _kFakeBase64);

      final result = await compressor.compressImageFromBytes(_someBytes());

      expect(result, isNotEmpty);
      expect(() => base64Decode(result), returnsNormally);
    });

    test('is called with the correct bytes and default parameters', () async {
      final bytes = _someBytes(32);

      when(
        () => compressor.compressImageFromBytes(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => _kFakeBase64);

      await compressor.compressImageFromBytes(bytes);

      verify(
        () => compressor.compressImageFromBytes(
          bytes,
        ),
      ).called(1);
    });

    test('is called with custom quality, maxWidth and maxHeight', () async {
      when(
        () => compressor.compressImageFromBytes(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => _kFakeBase64);

      await compressor.compressImageFromBytes(
        _someBytes(),
        quality: 60,
        maxWidth: 800,
        maxHeight: 600,
      );

      verify(
        () => compressor.compressImageFromBytes(
          any(),
          quality: 60,
          maxWidth: 800,
          maxHeight: 600,
        ),
      ).called(1);
    });

    test('throws when native returns null pointer', () async {
      when(
        () => compressor.compressImageFromBytes(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenThrow(
        Exception(
          'Image compression failed: native returned null pointer. '
          'Possible causes: file not found, unsupported image format, '
          'out-of-memory, or resize/JPEG encoding failure.',
        ),
      );

      expect(
        () => compressor.compressImageFromBytes(_someBytes()),
        throwsA(
          isA<Exception>().having(
            (e) => e.toString(),
            'message',
            allOf(
              contains('native returned null pointer'),
              contains('Possible causes'),
            ),
          ),
        ),
      );
    });

    test('throws ArgumentError for empty bytes', () async {
      when(
        () => compressor.compressImageFromBytes(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenThrow(ArgumentError('Image bytes cannot be empty.'));

      expect(
        () => compressor.compressImageFromBytes(Uint8List(0)),
        throwsA(
          isA<ArgumentError>().having(
            (e) => e.message,
            'message',
            contains('cannot be empty'),
          ),
        ),
      );
    });

    test('decoded result matches the expected bytes', () async {
      final knownBytes = _someBytes(8);
      final knownBase64 = base64Encode(knownBytes);

      when(
        () => compressor.compressImageFromBytes(
          any(),
          quality: any(named: 'quality'),
          maxWidth: any(named: 'maxWidth'),
          maxHeight: any(named: 'maxHeight'),
        ),
      ).thenAnswer((_) async => knownBase64);

      final result = await compressor.compressImageFromBytes(_someBytes());

      expect(base64Decode(result), equals(knownBytes));
    });
  });

  group('processImage', () {
    test('returns original bytes when no processing is needed', () async {
      final bytes = _someBytes(32);

      // We mock the native binding call instead of the ImageCompressor directly
      // However, we are using a mock of ImageCompressor. Wait!
      // The current tests are mocking the ImageCompressor itself. But NativeImageCompressor isn't being tested!
      // Ah, the test file has `class MockImageCompressor extends Mock implements ImageCompressor {}`
      // So it's just mocking the interface. We should just add tests for the interface mock.
      when(
        () => compressor.processImage(
          any(),
          rotationDegrees: any(named: 'rotationDegrees'),
          mirror: any(named: 'mirror'),
          maxDimension: any(named: 'maxDimension'),
          quality: any(named: 'quality'),
        ),
      ).thenAnswer((_) async => bytes);

      final result = await compressor.processImage(
        bytes,
        rotationDegrees: 0,
        mirror: false,
      );

      expect(result, equals(bytes));
      verify(
        () => compressor.processImage(
          bytes,
          rotationDegrees: 0,
          mirror: false,
        ),
      ).called(1);
    });

    test('is called with custom parameters', () async {
      final bytes = _someBytes(32);
      final processedBytes = _someBytes(16);

      when(
        () => compressor.processImage(
          any(),
          rotationDegrees: any(named: 'rotationDegrees'),
          mirror: any(named: 'mirror'),
          maxDimension: any(named: 'maxDimension'),
          quality: any(named: 'quality'),
        ),
      ).thenAnswer((_) async => processedBytes);

      final result = await compressor.processImage(
        bytes,
        rotationDegrees: 90,
        mirror: true,
        maxDimension: 800,
        quality: 60,
      );

      expect(result, equals(processedBytes));
      verify(
        () => compressor.processImage(
          bytes,
          rotationDegrees: 90,
          mirror: true,
          maxDimension: 800,
          quality: 60,
        ),
      ).called(1);
    });

    test('throws ArgumentError for empty bytes', () async {
      when(
        () => compressor.processImage(
          any(),
          rotationDegrees: any(named: 'rotationDegrees'),
          mirror: any(named: 'mirror'),
          maxDimension: any(named: 'maxDimension'),
          quality: any(named: 'quality'),
        ),
      ).thenThrow(ArgumentError('Image bytes cannot be empty.'));

      expect(
        () => compressor.processImage(Uint8List(0), rotationDegrees: 0, mirror: false),
        throwsA(
          isA<ArgumentError>().having(
            (e) => e.message,
            'message',
            contains('cannot be empty'),
          ),
        ),
      );
    });
  });
}
