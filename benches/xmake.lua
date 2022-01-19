add_requires("benchmark 1.5.5")

target("Benches")
    set_kind("binary")
    set_default(false)
    add_deps("Protocon")
    add_files("*.cpp")
    add_packages("benchmark")
