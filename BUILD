SYSROOT_COPTS = [
    "-isystem",
    "external/raspberry_pi/sysroot/usr/include",
]

cc_library(
    name = "libprojectm",
    srcs = glob(
        [
            "**/*.cpp",
            "**/*.c",
        ],
        exclude = ["omptl/Example.cpp"],
    ),
    hdrs = glob([
        "**/*.hpp",
        "**/*.h",
    ]) + [
        "omptl/omptl",
        "omptl/omptl_numeric",
        "omptl/omptl_algorithm",
    ],
    copts = SYSROOT_COPTS + [
        "-isystem",
        "libprojectm",
        "-isystem",
        "libprojectm/MilkdropPresetFactory",
        "-isystem",
        "libprojectm/NativePresetFactory",
        "-isystem",
        "libprojectm/omptl",
        "-isystem",
        "libprojectm/Renderer",
        "-isystem",
        "libprojectm/Renderer/SOIL2",
        "-isystem",
        "libprojectm/Renderer/hlslparser/src",
        "-DDATADIR_PATH=\\\"./\\\"",
        "-DUSE_THREADS",
    ],
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-lstdc++fs",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_absl//absl/types:span",
        "@org_llvm_libcxx//:libcxx",
    ],
)
