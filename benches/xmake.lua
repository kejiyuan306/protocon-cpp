add_requires("benchmark 1.5.5")

target("Benches")
    set_kind("binary")
    add_deps("Protocon")
    add_files("*.cpp")
    add_packages("benchmark")
