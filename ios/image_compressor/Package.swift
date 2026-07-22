// swift-tools-version: 5.9

import PackageDescription

let package = Package(
    name: "image_compressor",
    platforms: [
        .iOS(.v12)
    ],
    products: [
        .library(name: "image-compressor", targets: ["image_compressor"]),
    ],
    dependencies: [
        .package(name: "FlutterFramework", path: "../FlutterFramework"),
    ],
    targets: [
        .target(
            name: "image_compressor",
            dependencies: [
                .product(name: "FlutterFramework", package: "FlutterFramework"),
            ],
            // Sources/image_compressor/image_compressor.cpp is a symlink to
            // ../../../../src/image_compressor.cpp — no source duplication.
            //
            // Sources/image_compressor/include/ contains a copy of image_compressor.h.
            // SPM requires publicHeadersPath to be a flat directory (no subdirs),
            // so we cannot point directly at src/ which contains the tests/ subfolder.
            // This is the standard pattern used by Flutter plugins (e.g. FlutterFire).
            publicHeadersPath: "include",
            cxxSettings: [
                // Sources/image_compressor/ — for image_compressor.h
                .headerSearchPath("Sources/image_compressor"),
                // third_party/ symlink → ../../third_party — contains stb/ subfolder
                .headerSearchPath("third_party"),
                // Also add stb search path relative to the real source file location,
                // since Xcode resolves the symlink and compiles from src/.
                .unsafeFlags(["-I../../third_party"]),
            ]
        )
    ],
    cxxLanguageStandard: .cxx11
)
