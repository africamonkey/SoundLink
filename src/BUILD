load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "demo",
    srcs = ["demo.cpp"],
    data = [
        "//params:wav_params",
    ],
    deps = [
        "//src/common/file:io",
        "//src/common/math:math_utils",
        "//src/wav:wav_writer",
        "@glog",
    ],
)
