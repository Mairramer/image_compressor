# image_compressor.podspec

Pod::Spec.new do |s|
  s.name             = 'image_compressor'
  s.version          = '0.0.1'
  s.summary          = 'High-performance image compression using native C++ via Flutter FFI.'
  s.description      = <<-DESC
A Flutter plugin that performs fast JPEG compression using C++ via FFI.
                       DESC
  s.homepage         = 'https://github.com/your_username/image_compressor'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'Mairramer Veloso' => 'mairramer.dasilva28@hotmail.com' }
  s.source           = { :path => '.' }

  # Includes files in the Swift Package Manager Sources directory AND all .h/.cpp files from `src` and `third_party`.
  s.source_files = 'image_compressor/Sources/image_compressor/**/*.{h,m,mm}', '../src/**/*.{h,cpp}', '../third_party/stb/*.h'

  # Point the public header to its location in the `src` folder.
  s.public_header_files = 'image_compressor/Sources/image_compressor/include/**/*.h', '../src/image_compressor.h'

  s.dependency 'Flutter'
  s.platform = :ios, '12.0'

  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES',
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++11',
    'CLANG_CXX_LIBRARY' => 'libc++',
    # This helps Xcode find headers like "stb_image.h" from your .cpp file
    'HEADER_SEARCH_PATHS' => '"$(PODS_TARGET_SRCROOT)/../src" "$(PODS_TARGET_SRCROOT)/../third_party"'
  }
  s.swift_version = '5.0'
end
