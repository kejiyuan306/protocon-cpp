set_project("Protocon")
set_version("2.4.0", {build = "%Y%m%d%H%M"})

set_xmakever("2.5.9")

add_requires("asio 1.20.0")

set_warnings("all", "error")
set_languages("cxx20")

target("Protocon")
    set_kind("static")
    add_files("src/*.cpp")
    add_includedirs("include", { public = true })
    add_packages("asio")
    add_headerfiles("include/(Protocon/*.h)")

includes("tests")
includes("benches")
