Pod::Spec.new do |s|
  s.name             = 'image_compressor'
  s.version          = '0.0.1'
  s.summary          = 'High-performance image compression using native C++ via Flutter FFI.'
  s.description      = <<-DESC
image_compressor is a Flutter plugin that performs fast JPEG compression using C++,
accessible via FFI for both Android and iOS.
  DESC
  s.homepage         = 'https://github.com/your_username/image_compressor'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'Mairramer Veloso' => 'mairramer.dasilva28@hotmail.com' }

  s.source           = { :path => '.' }
  s.source_files     = 'Classes/**/*'
  s.dependency       'Flutter'
  s.platform         = :ios, '12.0'

  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES',
    'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'i386'
  }
  s.swift_version = '5.0'
end
