load("@com_google_protobuf//:protobuf.bzl", "py_proto_library")

VIDEOCORE_COPTS = [
    "-isystem",
    "external/raspberry_pi/sysroot/opt/vc/include",
    "-isystem",
    "external/raspberry_pi/sysroot/opt/vc/include/interface/vcos/pthreads",
    "-isystem",
    "external/raspberry_pi/sysroot/opt/vc/include/interface/vmcs_host/linux",
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
        ":vc_capture_source",
        ":projectm_controller",
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
        "-isystem",
        "external/raspberry_pi/sysroot/usr/include",
        "-Wthread-safety",
    ] + VIDEOCORE_COPTS,
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
