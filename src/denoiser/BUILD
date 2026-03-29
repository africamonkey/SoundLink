load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "denoiser_base",
    srcs = ["denoiser_base.cpp"],
    hdrs = ["denoiser_base.h"],
    deps = [
        "@glog//:glog",
    ],
)

cc_library(
    name = "simple_denoiser",
    srcs = ["simple_denoiser.cpp"],
    hdrs = ["simple_denoiser.h"],
    deps = [
        ":denoiser_base",
        "@glog//:glog",
    ],
)

cc_test(
    name = "simple_denoiser_test",
    srcs = ["simple_denoiser_test.cpp"],
    data = [
        "//params:encoder_params",
        "//params:wav_params",
    ],
    deps = [
        ":simple_denoiser",
        "//src/common/file:io",
        "//src/common/interface/proto:cc_encoder_params_proto",
        "//src/common/interface/proto:cc_wav_params_proto",
        "//src/common/testing:google_test_main",
        "//src/encoder:trival_encoder",
        "//src/wav:wav_noise_producer",
        "//src/wav:wav_reader",
        "//src/wav:wav_writer",
        "@gflags//:gflags",
        "@glog//:glog",
    ],
)
