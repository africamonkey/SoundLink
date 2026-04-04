load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "receiver",
    srcs = ["receiver.cpp"],
    hdrs = ["receiver.h"],
    deps = [
        "//src/audio:audio",
        "//src/encoder:encoder_base",
        "@glog//:glog",
    ],
)

cc_binary(
    name = "receiver_main",
    srcs = ["receiver_main.cpp"],
    data = ["//params:encoder_params.txt"],
    deps = [
        ":receiver",
        "//src/common/file:io",
        "//src/common/interface/proto:cc_encoder_params_proto",
        "//src/encoder:chirp_encoder",
        "//src/encoder:goertzel_encoder",
        "//src/encoder:simple_encoder",
        "@gflags//:gflags",
        "@glog//:glog",
    ],
)

cc_binary(
    name = "receiver_recorder_main",
    srcs = ["receiver_recorder_main.cpp"],
    deps = [
        "//src/audio:audio",
        "//src/common/file:io",
        "//src/common/interface/proto:cc_wav_params_proto",
        "//src/wav:wav_writer",
        "@gflags//:gflags",
        "@glog//:glog",
    ],
)

cc_test(
    name = "receiver_test",
    srcs = ["receiver_test.cpp"],
    deps = [
        ":receiver",
        "//src/common/testing:google_test_main",
        "//src/encoder:encoder_base",
        "@glog//:glog",
        "@googletest//:gtest",
    ],
)
