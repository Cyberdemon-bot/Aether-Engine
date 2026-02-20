includes("packages/*.lua")

add_requires("spdlog", "fmt", "glm", "entt", "yaml-cpp", "glfw")
add_requires("imgui v1.89.9-docking", {configs = {glfw_opengl3 = true}})
add_requires("imguizmo", {configs = {imgui = "imgui"}})
add_requires("stb", "cgltf", "joltphysics", "filewatch", "glad", "ozz-animation")

add_requireconfs("freetype", {
    override = true,
    configs = { bzip2 = false, png = false, brotli = false, zlib = false }
})

add_requireconfs("msdf-atlas-gen", {
    configs = { shared = false, freetype = true }
})

if is_plat("mingw") then
    add_requireconfs("*", {
        configs = {
            cxflags = "-pthread",
            ldflags = {"-pthread", "-lpthread"}
        }
    })
end

target("Aether")
    set_kind("shared")
    set_languages("cxx17")

    add_defines("AETHER_SHARED")
    add_defines("AETHER_BUILD_DLL")
    add_defines("MSDFGEN_USE_CPP11", "MSDFGEN_EXTENSIONS")

    if is_mode("debug") then
        add_defines("AETHER_DEBUG", {public = true})
        set_symbols("debug")
        set_policy("build.sanitizer.address", true)
    end

    add_includedirs("src", {public = true})
    add_includedirs("vendor", {public = true})

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    set_pcheader("src/aepch.h")

    add_packages("spdlog", "fmt", "glm", "entt", "yaml-cpp", "glfw", "imgui", "stb", "imguizmo", "freetype", "cgltf", {public = true})
    add_packages("filewatch", "msdf-atlas-gen", "glad", "joltphysics", "ozz-animation", {public = true})

    if is_plat("mingw") then
        add_syslinks("pthread", {public = true})
        add_cxflags("-finput-charset=UTF-8", {public = true})
    end

    if is_plat("windows") then
        add_cxflags("/utf-8", {public = true})
    end

    if is_os("windows") then
        add_syslinks("opengl32", {public = true})
    elseif is_os("macosx") then
        add_frameworks("OpenGL", "Cocoa", "IOKit", "CoreVideo", {public = true})
    elseif is_os("linux") then
        add_syslinks("pthread", "dl", {public = true})
    end