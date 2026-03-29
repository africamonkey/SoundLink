load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "sender",
    srcs = ["sender.cpp"],
    hdrs = ["sender.h"],
    deps = [
        "//src/audio:audio",
        "//src/encoder:encoder_base",
        "@glog//:glog",
    ],
)

cc_binary(
    name = "sender_main",
    srcs = ["main.cpp"],
    deps = [
        ":sender",
        "//src/common/file:io",
        "//src/common/interface/proto:cc_encoder_params_proto",
        "//src/encoder:simple_encoder",
        "@gflags//:gflags",
        "@glog//:glog",
    ],
)
