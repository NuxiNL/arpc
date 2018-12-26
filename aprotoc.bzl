def aprotoc(name, src):
    native.genrule(
        name = name,
        srcs = [src],
        outs = [name + ".ad.h"],
        cmd = "./$(location @com_github_nuxinl_arpc//scripts:aprotoc) < $(location %s) > $@" % src,
        tools = ["@com_github_nuxinl_arpc//scripts:aprotoc"],
    )
