package(default_visibility = ["//visibility:public"])

cc_library(
    name = "wav_header",
    hdrs = ["wav_header.h"],
    deps = [
        "@glog//:glog",
    ],
)

cc_library(
    name = "wav_reader",
    srcs = ["wav_reader.cpp"],
    hdrs = ["wav_reader.h"],
    deps = [
        ":wav_header",
        "@glog//:glog",
    ],
)

cc_test(
    name = "wav_reader_test",
    srcs = ["wav_reader_test.cpp"],
    data = [
        "//params:wav_params",
    ],
    deps = [
        ":wav_reader",
        ":wav_writer",
        "//src/common/file:io",
        "//src/common/testing:google_test_main",
    ],
)

cc_library(
    name = "wav_writer",
    srcs = ["wav_writer.cpp"],
    hdrs = ["wav_writer.h"],
    deps = [
        ":wav_header",
        "//src/common/interface/proto:cc_wav_params_proto",
        "//src/common/math:math_utils",
    ],
)

cc_test(
    name = "wav_writer_test",
    srcs = ["wav_writer_test.cpp"],
    data = [
        "//params:wav_params",
    ],
    deps = [
        ":wav_writer",
        "//src/common/file:io",
        "//src/common/math:math_utils",
        "//src/common/testing:google_test_main",
    ],
)
