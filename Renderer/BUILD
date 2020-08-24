load("//libprojectm:variables.bzl", "PROJECTM_COPTS", "SYSROOT_COPTS")

cc_library(
    name = "renderer",
    srcs = glob(
        [
            "**/*.cpp",
            "**/*.c",
        ],
        exclude = [
            "Renderer/TextureManager.cpp",
            "Renderer/Texture.cpp",
            "Renderer/Shader.cpp",
            "Renderer/PerlinNoiseWithAlpha.cpp",
            "Renderer/ShaderEngine.cpp",
            "Renderer/Renderable.cpp",
            "Renderer/BeatDetect.cpp",
            "Renderer/hlslparser/*",
            "SOIL2/*",
        ],
    ),
    hdrs = glob(
        [
            "**/*.hpp",
            "**/*.h",
        ],
        exclude = [
            "Renderer/TextureManager.hpp",
            "Renderer/Texture.hpp",
            "Renderer/Shader.hpp",
            "Renderer/PerlinNoiseWithAlpha.hpp",
            "Renderer/ShaderEngine.hpp",
            "Renderer/Renderable.hpp",
            "Renderer/BeatDetect.hpp",
            "Renderer/hlslparser/*",
            "SOIL2/*",
        ],
    ),
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":shader",
        ":shader_engine",
        ":static_gl_shaders",
        ":texture",
        ":texture_manager",
        "//libprojectm:libprojectm_headers",
        "//libprojectm/Renderer/SOIL2:soil2",
        "@com_google_absl//absl/types:span",
        "@com_google_absl//absl/strings",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "hlslparser",
    srcs = glob([
        "hlslparser/**/*.cpp",
        "hlslparser/**/*.c",
    ]),
    hdrs = glob([
        "hlslparser/**/*.hpp",
        "hlslparser/**/*.h",
    ]),
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "beat_detect",
    srcs = ["BeatDetect.cpp"],
    hdrs = ["BeatDetect.hpp"],
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//libprojectm:libprojectm_headers",
        "//libprojectm:wipemalloc",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "per_pixel_mesh",
    srcs = ["PerPixelMesh.cpp"],
    hdrs = ["PerPixelMesh.hpp"],
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//libprojectm:libprojectm_headers",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "static_shaders",
    srcs = ["StaticShaders.cpp"],
    hdrs = ["StaticShaders.hpp"],
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":shader",
        ":static_gl_shaders",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "static_gl_shaders",
    srcs = ["StaticGlShaders.cpp"],
    hdrs = ["StaticGlShaders.h"],
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":hlslparser",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "renderable",
    srcs = ["Renderable.cpp"],
    hdrs = ["Renderable.hpp"],
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":static_shaders",
        ":texture",
        ":texture_manager",
        "//libprojectm:libprojectm_headers",
        "//libprojectm:wipemalloc",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "pipeline",
    srcs = ["Pipeline.cpp"],
    hdrs = ["Pipeline.hpp"],
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":per_pixel_mesh",
        ":renderable",
        ":shader",
        "//libprojectm:libprojectm_headers",
        "//libprojectm:wipemalloc",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "shader_engine",
    srcs = ["ShaderEngine.cpp"],
    hdrs = ["ShaderEngine.hpp"],
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":beat_detect",
        ":hlslparser",
        ":pipeline",
        ":shader",
        ":static_shaders",
        ":texture",
        ":texture_manager",
        "//libprojectm:libprojectm_headers",
        "@com_google_absl//absl/types:span",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "shader",
    srcs = ["Shader.cpp"],
    hdrs = ["Shader.hpp"],
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":texture",
        ":texture_manager",
        "//libprojectm:libprojectm_headers",
        "@com_google_absl//absl/types:span",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "texture",
    srcs = ["Texture.cpp"],
    hdrs = ["Texture.hpp"],
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//libprojectm:libprojectm_headers",
        "@com_google_absl//absl/types:span",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "texture_manager",
    srcs = [
        "PerlinNoiseWithAlpha.cpp",
        "TextureManager.cpp",
    ],
    hdrs = [
        "PerlinNoiseWithAlpha.hpp",
        "TextureManager.hpp",
    ],
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":texture",
        "//libprojectm:libprojectm_headers",
        "//libprojectm/Renderer/SOIL2:soil2",
        "@com_google_absl//absl/types:span",
        "@org_llvm_libcxx//:libcxx",
    ],
)
