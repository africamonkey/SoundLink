load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "audio",
    srcs = [
        "audio_player.cpp",
        "audio_capturer.cpp",
    ],
    hdrs = [
        "audio_player.h",
        "audio_capturer.h",
    ],
    linkopts = select({
        "@platforms//os:macos": ["-L/opt/homebrew/lib", "-lportaudio", "-lpthread"],
        "//conditions:default": ["-lportaudio", "-lpthread"],
    }),
    deps = [
        "//third_party/portaudio",
        "@glog//:glog",
    ],
)
