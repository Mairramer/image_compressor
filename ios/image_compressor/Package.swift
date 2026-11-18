// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "image_compressor",
    platforms: [
        .iOS("12.0")
    ],
    products: [
        .library(name: "image-compressor", type: .dynamic, targets: ["image_compressor"])
    ],
    dependencies: [
        .package(name: "FlutterFramework", path: "../FlutterFramework")
    ],
    targets: [
        .target(
            name: "image_compressor",
            dependencies: [
                .product(name: "FlutterFramework", package: "FlutterFramework")
            ],
            publicHeadersPath: "include"
        )
    ],
    cxxLanguageStandard: .cxx11
)
