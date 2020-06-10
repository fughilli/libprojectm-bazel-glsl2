load("//libprojectm:variables.bzl", "PROJECTM_COPTS", "SYSROOT_COPTS")

cc_library(
    name = "soil2",
    srcs = glob(
        [
            "**/*.cpp",
            "**/*.c",
        ],
    ),
    hdrs = glob(
        [
            "**/*.hpp",
            "**/*.h",
        ],
    ),
    copts = SYSROOT_COPTS + PROJECTM_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "@org_llvm_libcxx//:libcxx",
    ],
)
