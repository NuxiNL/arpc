def aprotoc(name, src):
    # Output header file name.
    header_name = src
    if header_name.endswith(".proto"):
        header_name = header_name[:-6]
    header_name += ".ad.h"

    native.genrule(
        name = name,
        srcs = [src],
        outs = [header_name],
        cmd = "./$(location @org_cloudabi_arpc//scripts:aprotoc) < $(location %s) > $@" % src,
        tools = ["@org_cloudabi_arpc//scripts:aprotoc"],
    )
