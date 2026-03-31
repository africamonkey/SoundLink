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

cc_library(
    name = "audio_normalizer",
    srcs = ["audio_normalizer.cpp"],
    hdrs = ["audio_normalizer.h"],
    deps = [
        "@glog//:glog",
    ],
)

cc_test(
    name = "audio_normalizer_test",
    srcs = ["audio_normalizer_test.cpp"],
    data = [
        "//params:encoder_params",
        "//params:wav_params",
    ],
    deps = [
        ":audio_normalizer",
        "//src/common/file:io",
        "//src/common/interface/proto:cc_encoder_params_proto",
        "//src/common/interface/proto:cc_wav_params_proto",
        "//src/common/testing:google_test_main",
        "//src/encoder:simple_encoder",
        "//src/wav:wav_reader",
        "//src/wav:wav_writer",
        "@gflags//:gflags",
        "@glog//:glog",
    ],
)

cc_binary(
    name = "audio_denoise_and_normalize_main",
    srcs = ["audio_denoise_and_normalize_main.cpp"],
    data = [
        "//src/denoiser/test_data:test_data",
    ],
    deps = [
        ":audio_normalizer",
        ":simple_denoiser",
        "//src/common/file:io",
        "//src/wav:wav_reader",
        "//src/wav:wav_writer",
        "@gflags//:gflags",
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
        "//src/encoder:simple_encoder",
        "//src/wav:wav_noise_producer",
        "//src/wav:wav_reader",
        "//src/wav:wav_writer",
        "@gflags//:gflags",
        "@glog//:glog",
    ],
)
