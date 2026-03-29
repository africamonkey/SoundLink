load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "io_test_proto",
    srcs = ["io_test.proto"],
)

cc_proto_library(
    name = "cc_io_test_proto",
    deps = [":io_test_proto"],
)

proto_library(
    name = "wav_params_proto",
    srcs = ["wav_params.proto"],
)

cc_proto_library(
    name = "cc_wav_params_proto",
    deps = [":wav_params_proto"],
)

proto_library(
    name = "encoder_params_proto",
    srcs = ["encoder_params.proto"],
)

cc_proto_library(
    name = "cc_encoder_params_proto",
    deps = [":encoder_params_proto"],
)
