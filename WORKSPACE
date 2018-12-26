workspace(name = "com_github_nuxinl_arpc")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "com_github_nuxinl_argdata",
    commit = "6299455171a28831876d078c59a6634de6f6700b",
    remote = "https://github.com/NuxiNL/argdata.git",
)

git_repository(
    name = "com_google_googletest",
    commit = "150613166524c474a8a97df4c01d46b72050c495",
    remote = "https://github.com/google/googletest.git",
)

git_repository(
    name = "io_bazel_rules_python",
    commit = "e6399b601e2f72f74e5aa635993d69166784dde1",
    remote = "https://github.com/bazelbuild/rules_python.git",
)

load("@io_bazel_rules_python//python:pip.bzl", "pip_import", "pip_repositories")

pip_repositories()

pip_import(
    name = "aprotoc_deps",
    requirements = "//scripts:requirements.txt",
)

load("@aprotoc_deps//:requirements.bzl", "pip_install")

pip_install()
