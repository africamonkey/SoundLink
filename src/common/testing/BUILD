package(default_visibility = ["//visibility:public"])

cc_library(
    name = "google_test_main",
    testonly = 1,
    srcs = ["google_test_main.cpp"],
    deps = [
        "@glog//:glog",
        "@googletest//:gtest",
        "@gflags//:gflags",
    ],
    alwayslink = 1,
)
