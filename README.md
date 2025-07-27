# image_compressor

A Flutter plugin for efficient JPEG image compression using native C/C++ code via FFI for Android and iOS.

---

## Overview

`image_compressor` is a Flutter plugin that delivers fast, high-quality image compression by leveraging native C/C++ code accessed through **FFI** (Foreign Function Interface).

This project provides a solid foundation for native image compression in Flutter apps, featuring:

- Optimized JPEG compression
- Smart resizing based on max dimensions
- Simple Dart API for easy integration

---

## Installation

Add the following to your `pubspec.yaml`:

```yaml
dependencies:
  image_compressor:
    git:
      url: https://github.com/Mairramer/image_compressor.git
```

Then run:

```bash
flutter pub get
```

---

## Basic Usage

```dart
import 'package:image_compressor/image_compressor.dart';

final imageCompressor = ImageCompressor();

final compressedBase64 = await imageCompressor.compressImageFromPath(
  '/path/to/image.jpg',
  quality: 70,
  maxSize: 1080,
);
```

This method returns the compressed image encoded as a base64 string.

---

## Project Structure

- **`src/`**: Native C/C++ source code and CMakeLists for building native libraries.
- **`lib/`**: Dart code exposing the public API and FFI bindings.
- **`android/`, `ios/`**: Platform-specific build configurations for bundling native binaries.
- **`example/`**: A Flutter example app demonstrating plugin usage.

---

## Native Build and Packaging

The plugin automatically compiles and packages native libraries for Android and iOS using:

- **Android**: Gradle + Android NDK
- **iOS**: CocoaPods + Xcode
- **Windows/Linux/macOS** (planned): CMake

Configured in `pubspec.yaml` as:

```yaml
plugin:
  platforms:
    android:
      ffiPlugin: true
    ios:
      ffiPlugin: true
```

---

## Binding Generation

Dart bindings to the native code are auto-generated using [package:ffigen](https://pub.dev/packages/ffigen).

Regenerate bindings with:

```bash
dart run ffigen --config ffigen.yaml
```

---

## Invoking Native Code

- Short-running native functions can be called directly via FFI.
- Longer-running operations (e.g., image compression) should run in isolates to keep UI smooth.

See `lib/image_compressor.dart` for usage examples.

---

## Flutter Documentation

For more info on Flutter and plugin development:

- [Flutter Official Docs](https://docs.flutter.dev)
- [Flutter FFI Guide](https://flutter.dev/docs/development/platform-integration/c-interop)
- [Official Flutter FFI Plugin Example](https://github.com/flutter/plugins/tree/main/packages/ffi/example)
