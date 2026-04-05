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
        "//src/encoder/simple_encoder_test_data:simple_encoder_easy",
        "//src/encoder/simple_encoder_test_data:simple_encoder_medium",
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

cc_library(
    name = "goertzel_encoder",
    srcs = ["goertzel_encoder.cpp"],
    hdrs = ["goertzel_encoder.h"],
    deps = [
        ":encoder_base",
        "//src/common/math:math_utils",
        "@glog//:glog",
    ],
)

cc_library(
    name = "chirp_encoder",
    srcs = ["chirp_encoder.cpp"],
    hdrs = ["chirp_encoder.h"],
    deps = [
        ":encoder_base",
        "//src/common/math:fft",
        "//src/common/math:math_utils",
        "@glog//:glog",
    ],
)

cc_test(
    name = "goertzel_encoder_test",
    srcs = ["goertzel_encoder_test.cpp"],
    data = [
        "//params:wav_params",
        "//src/encoder/goertzel_encoder_test_data:goertzel_easy",
        "//src/encoder/goertzel_encoder_test_data:goertzel_easy_noisy",
        "//src/encoder/goertzel_encoder_test_data:goertzel_easy_noisy_plus",
        "//src/encoder/goertzel_encoder_test_data:goertzel_medium",
    ],
    deps = [
        ":goertzel_encoder",
        "//src/common/file:io",
        "//src/common/interface/proto:cc_wav_params_proto",
        "//src/common/testing:google_test_main",
        "//src/wav:wav_reader",
        "//src/wav:wav_writer",
        "@glog//:glog",
    ],
)

cc_test(
    name = "chirp_encoder_test",
    srcs = ["chirp_encoder_test.cpp"],
    data = [
        "//params:encoder_params",
        "//params:wav_params",
    ],
    deps = [
        ":chirp_encoder",
        "//src/common/file:io",
        "//src/common/interface/proto:cc_wav_params_proto",
        "//src/common/testing:google_test_main",
        "//src/wav:wav_reader",
        "//src/wav:wav_writer",
        "//src/wav:wav_noise_producer",
        "@glog//:glog",
    ],
)
