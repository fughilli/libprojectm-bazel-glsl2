SYSROOT_COPTS = [
    "-isystem",
    "external/raspberry_pi/sysroot/usr/include",
]
PROJECTM_COPTS = [
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
]
