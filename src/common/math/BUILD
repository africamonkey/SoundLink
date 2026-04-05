load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "math_utils",
    srcs = ["math_utils.cpp"],
    hdrs = ["math_utils.h"],
    deps = [
        "@glog//:glog",
    ],
)

cc_library(
    name = "fft",
    srcs = ["fft.cpp"],
    hdrs = ["fft.h"],
    deps = [
        "@glog//:glog",
    ],
)

cc_test(
    name = "fft_test",
    srcs = ["fft_test.cpp"],
    deps = [
        ":fft",
        ":math_utils",
        "//src/common/testing:google_test_main",
    ],
)
