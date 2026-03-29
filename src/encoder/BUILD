load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "encoder_base",
    srcs = ["encoder_base.cpp"],
    hdrs = ["encoder_base.h"],
    deps = [
        "//src/common/interface/proto:cc_encoder_params_proto",
        "@glog//:glog",
    ],
)

cc_library(
    name = "simple_encoder",
    srcs = ["simple_encoder.cpp"],
    hdrs = ["simple_encoder.h"],
    deps = [
        ":encoder_base",
        "//src/common/math:math_utils",
        "@glog//:glog",
    ],
)

cc_test(
    name = "simple_encoder_test",
    srcs = ["simple_encoder_test.cpp"],
    data = [
        "//params:encoder_params",
        "//params:wav_params",
        "//src/encoder/test_data:easy",
        "//src/encoder/test_data:medium",
    ],
    deps = [
        ":simple_encoder",
        "//src/common/file:io",
        "//src/common/interface/proto:cc_wav_params_proto",
        "//src/common/testing:google_test_main",
        "//src/wav:wav_reader",
        "//src/wav:wav_writer",
        "@glog//:glog",
    ],
)
