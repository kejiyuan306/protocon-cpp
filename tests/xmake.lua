add_requires("gtest 1.11.0")

target("Tests")
    set_kind("binary")
    set_default(false)
    add_deps("Protocon")
    add_files("*.cpp")
    add_packages("gtest")
