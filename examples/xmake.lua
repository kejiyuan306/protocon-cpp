target("Print")
    set_kind("binary")
    set_default("false")
    add_deps("Protocon")
    add_files("Print.cpp")
    add_ldflags("-pthread")
    add_packages("spdlog")
