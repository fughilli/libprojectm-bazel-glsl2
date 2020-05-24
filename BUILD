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
        exclude = [
            "omptl/Example.cpp",
            "TestRunner.cpp",
        ],
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
        "-DLINUX",
        "-DprojectM_FONT_TITLE=\\\"\\\"",
        "-DprojectM_FONT_MENU=\\\"\\\"",
        "-DCMAKE_INSTALL_PREFIX=\\\"\\\"",

    ],
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "@org_llvm_libcxx//:libcxx",
    ],
)
