load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "google_test_main",
    testonly = 1,
    srcs = ["google_test_main.cpp"],
    deps = [
        "@gflags",
        "@glog",
        "@googletest//:gtest",
    ],
    alwayslink = 1,
)
