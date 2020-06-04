load("@com_google_protobuf//:protobuf.bzl", "py_proto_library")
load("//:py_wrap_cc.bzl", "py_wrap_cc")
load("@subpar//:subpar.bzl", "par_binary")
load("//ugfx:variables.bzl", "UGFX_COPTS")

PYWRAP_COPTS = select({
    "//:pi_build": [
        "-isystem",
        "external/raspberry_pi/sysroot/usr/include/python3.5m",
        "-isystem",
        "external/raspberry_pi/sysroot/usr/include/arm-linux-gnueabihf/python3.5m",
    ],
    "//conditions:default": [
        "-isystem",
        "/usr/include/python3.7m",
    ],
})

VIDEOCORE_COPTS = [
    "-isystem",
    "external/raspberry_pi/sysroot/opt/vc/include",
    "-isystem",
    "external/raspberry_pi/sysroot/opt/vc/include/interface/vcos/pthreads",
    "-isystem",
    "external/raspberry_pi/sysroot/opt/vc/include/interface/vmcs_host/linux",
]

SYSROOT_COPTS = [
    "-isystem",
    "external/raspberry_pi/sysroot/usr/include",
]

cc_library(
    name = "vc_capture_source",
    srcs = ["vc_capture_source.cc"],
    hdrs = ["vc_capture_source.h"],
    copts = [
        "-Wthread-safety",
    ] + VIDEOCORE_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-Lexternal/raspberry_pi/sysroot/opt/vc/lib",
        "-lbcm_host",
    ],
    linkstatic = 1,
)

cc_library(
    name = "spi_driver",
    srcs = ["spi_driver.cc"],
    hdrs = ["spi_driver.h"],
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkstatic = 1,
)

cc_library(
    name = "periodic",
    hdrs = ["periodic.h"],
)

cc_library(
    name = "performance_timer",
    hdrs = ["performance_timer.h"],
)

cc_library(
    name = "pixel_utils",
    hdrs = ["pixel_utils.h"],
)

cc_library(
    name = "visual_interest_processor",
    srcs = ["visual_interest_processor.cc"],
    hdrs = ["visual_interest_processor.h"],
    copts = [
        "-Wthread-safety",
    ] + VIDEOCORE_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-Lexternal/raspberry_pi/sysroot/opt/vc/lib",
        "-lbcm_host",
    ],
    linkstatic = 1,
    deps = [
        ":periodic",
        ":projectm_controller",
        ":vc_capture_source",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "projectm_controller",
    srcs = ["projectm_controller.cc"],
    hdrs = ["projectm_controller.h"],
    copts = [
        "-isystem",
        "external/raspberry_pi/sysroot/usr/include",
    ],
    linkopts = ["-lxdo"],
    linkstatic = 1,
)

cc_binary(
    name = "led_driver",
    srcs = ["led_driver.cc"],
    copts = [
        "-Wthread-safety",
    ] + SYSROOT_COPTS + VIDEOCORE_COPTS,
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    linkopts = [
        "-Lexternal/raspberry_pi/sysroot/opt/vc/lib",
        "-lbcm_host",
        "-lxdo",
    ],
    linkstatic = 1,
    deps = [
        ":led_mapping_cc_proto",
        ":pixel_utils",
        ":projectm_controller",
        ":spi_driver",
        ":vc_capture_source",
        ":visual_interest_processor",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@org_llvm_libcxx//:libcxx",
    ],
)

proto_library(
    name = "led_mapping_proto",
    srcs = ["led_mapping.proto"],
)

cc_proto_library(
    name = "led_mapping_cc_proto",
    deps = [":led_mapping_proto"],
)

py_proto_library(
    name = "led_mapping_py_proto",
    srcs = ["led_mapping.proto"],
)

py_binary(
    name = "mapping_generator",
    srcs = ["mapping_generator.py"],
    data = ["generate.py"],
    deps = [
        ":led_mapping_py_proto",
    ],
)

cc_binary(
    name = "projectm_sdl_test",
    srcs = ["projectm_sdl_test.cc"],
    copts = SYSROOT_COPTS,
    linkopts = [
        "-Wl,-z,notext",
        "-lSDL2",
        "-lasound",
        "-lpulse",
        "-lX11",
        "-lXext",
        "-lXt",
        "-lsndio",
        "-lXcursor",
        "-lXinerama",
        "-lXrandr",
        "-lwayland-cursor",
        "-lwayland-client",
        "-lwayland-egl",
        "-lxkbcommon",
        "-lXxf86vm",
        "-lXss",
        "-lXi",
        "-lGL",
    ],
    linkstatic = 1,
    deps = [
        ":performance_timer",
        ":pulseaudio_interface",
        "//libprojectm",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/types:span",
        "@org_llvm_libcxx//:libcxx",
    ],
)

cc_library(
    name = "pulseaudio_interface",
    srcs = ["pulseaudio_interface.cc"],
    hdrs = ["pulseaudio_interface.h"],
    linkopts = [
        "-lpulse",
    ],
    linkstatic = 1,
    deps = [
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "display_driver",
    srcs = ["display_driver.cc"],
    hdrs = ["display_driver.h"],
    linkstatic = 1,
    deps = [
        "//HexapodController2:bus",
        "@com_google_absl//absl/types:span",
    ],
)

py_wrap_cc(
    name = "pywrap_display_driver",
    srcs = ["pywrap_display_driver.i"],
    copts = PYWRAP_COPTS,
    deps = [
        ":display_driver",
        "@org_llvm_libcxx//:libcxx",
    ],
)

par_binary(
    name = "py_display_driver",
    srcs = ["py_display_driver.py"],
    exclude_prefixes = [
        "external/raspberry_pi/sysroot",
    ],
    zip_safe = False,
    deps = [":pywrap_display_driver"],
)

cc_library(
    name = "test_lib",
    hdrs = ["test_lib.h"],
    linkstatic = 1,
)

py_wrap_cc(
    name = "pywrap_test_lib",
    srcs = ["pywrap_test_lib.i"],
    copts = PYWRAP_COPTS,
    deps = [
        ":test_lib",
        "@org_llvm_libcxx//:libcxx",
    ],
)

par_binary(
    name = "py_test_lib",
    srcs = ["py_test_lib.py"],
    zip_safe = False,
    deps = [":pywrap_test_lib"],
)

cc_binary(
    name = "test_display_driver",
    srcs = ["test_display_driver.cc"],
    copts = UGFX_COPTS + SYSROOT_COPTS,
    linkstatic = 1,
    deps = [
        ":display_driver",
        "//HexapodController2:bus",
        "//ugfx",
        "@org_llvm_libcxx//:libcxx",
    ],
)
