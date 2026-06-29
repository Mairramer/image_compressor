// swift-tools-version: 5.0
import PackageDescription

let package = Package(
    name: "image_compressor",
    platforms: [
        .iOS(.v12)
    ],
    products: [
        .library(
            name: "image_compressor",
            targets: ["image_compressor"]
        ),
    ],
    dependencies: [],
    targets: [
        .target(
            name: "image_compressor",
            dependencies: [],
            path: ".",
            sources: [
                "Classes",
                "src",
                "third_party"
            ],
            publicHeadersPath: "src", // Exposes image_compressor.h safely
            cxxSettings: [
                .headerSearchPath("src"),
                .headerSearchPath("third_party")
            ]
        )
    ],
    cxxLanguageStandard: .cxx17
)