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

  # 💡 This is the key change.
  # It includes any files in `ios/Classes` AND all .h/.cpp files from the external `src` folder.
  s.source_files = 'Classes/**/*.{h,m,mm}', '../src/**/*.{h,cpp}'

  # 💡 Point the public header to its location in the `src` folder.
  s.public_header_files = '../src/image_compressor.h'

  s.dependency 'Flutter'
  s.platform = :ios, '12.0'

  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES',
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'CLANG_CXX_LIBRARY' => 'libc++',
    # This helps Xcode find headers like "stb_image.h" from your .cpp file
    'HEADER_SEARCH_PATHS' => '"$(PODS_TARGET_SRCROOT)/src"'
  }
  s.swift_version = '5.0'
end
