load("//libprojectm:variables.bzl", "PROJECTM_COPTS", "SYSROOT_COPTS")

cc_library(
    name = "libprojectm_headers",
    hdrs = glob(
        [
            "**/*.hpp",
            "**/*.h",
        ],
        exclude = [
            "Renderer/**/*",
            "wipe*",
        ],
    ) + [
        "omptl/omptl",
        "omptl/omptl_numeric",
        "omptl/omptl_algorithm",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "wipemalloc",
    srcs = ["wipemalloc.cpp"],
    hdrs = ["wipemalloc.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "libprojectm",
    srcs = glob(
        [
            "**/*.cpp",
            "**/*.c",
        ],
        exclude = [
            "omptl/Example.cpp",
            "Renderer/**/*",
            "wipe*",
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
        ":libprojectm_headers",
        ":wipemalloc",
        "//libprojectm/Renderer:pipeline",
        "//libprojectm/Renderer:renderer",
        "//libprojectm/Renderer:shader",
        "//libprojectm/Renderer:texture",
        "//libprojectm/Renderer:texture_manager",
        "@com_google_absl//absl/types:span",
        "@org_llvm_libcxx//:libcxx",
    ],
)
