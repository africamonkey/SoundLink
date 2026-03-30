load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "receiver",
    srcs = ["receiver.cpp"],
    hdrs = ["receiver.h"],
    deps = [
        "//src/audio:audio",
        "//src/denoiser:simple_denoiser",
        "//src/encoder:encoder_base",
        "@glog//:glog",
    ],
)

cc_binary(
    name = "receiver_main",
    srcs = ["receiver_main.cpp"],
    deps = [
        ":receiver",
        "//src/common/file:io",
        "//src/common/interface/proto:cc_encoder_params_proto",
        "//src/encoder:goertzel_encoder",
        "//src/encoder:simple_encoder",
        "@gflags//:gflags",
        "@glog//:glog",
    ],
)
