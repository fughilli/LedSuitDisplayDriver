cc_library(
    name = "vc_capture_source",
    srcs = ["vc_capture_source.cc"],
    hdrs = ["vc_capture_source.h"],
    deps = ["@org_llvm_libcxx//:libcxx"],
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    copts = ["-isystem", "external/raspberry_pi/sysroot/opt/vc/include", "-isystem", "external/raspberry_pi/sysroot/opt/vc/include/interface/vcos/pthreads", "-isystem", "external/raspberry_pi/sysroot/opt/vc/include/interface/vmcs_host/linux", "-Wthread-safety"],
    linkopts = ["-Lexternal/raspberry_pi/sysroot/opt/vc/lib", "-lbcm_host"],
    linkstatic = 1,
)

cc_binary(
    name = "led_driver",
    srcs = ["led_driver.cc"],
    deps = ["@org_llvm_libcxx//:libcxx", ":vc_capture_source"],
    data = ["//tools/cc_toolchain/raspberry_pi_sysroot:everything"],
    copts = ["-isystem", "external/raspberry_pi/sysroot/opt/vc/include", "-isystem", "external/raspberry_pi/sysroot/opt/vc/include/interface/vcos/pthreads", "-isystem", "external/raspberry_pi/sysroot/opt/vc/include/interface/vmcs_host/linux", "-Wthread-safety"],
    linkopts = ["-Lexternal/raspberry_pi/sysroot/opt/vc/lib", "-lbcm_host"],
    linkstatic = 1,
)
