includes("packages/*.lua")

add_requires("spdlog", "fmt", "glm", "entt", "yaml-cpp", "glfw")
add_requires("imgui v1.89.9-docking", {configs = {glfw_opengl3 = true}})
add_requires("imguizmo", {configs = {imgui = "imgui"}})
add_requires("stb")
add_requires("assimp", {configs = {shared = true}})
add_requires("joltphysics")
add_requireconfs("freetype", {
    override = true, 
    configs = {
        bzip2 = false,
        png   = false,
        brotli= false,
        zlib  = false, 
    }
})

add_requireconfs("msdf-atlas-gen", {
    configs = {
        shared = false, 
        freetype = true 
    }
})
add_requires("filewatch", "glad")

target("Aether")
    set_kind("shared")
    set_languages("cxx17")
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

    add_packages("spdlog", "fmt", "glm", "entt", "yaml-cpp", "glfw", "imgui", "stb", "imguizmo", "freetype", "assimp", {public = true})
    add_packages("filewatch", "msdf-atlas-gen", "glad", "joltphysics", {public = true})

    if is_plat("mingw") then
        add_syslinks("pthread") 
    end

    if is_os("windows") then
        add_syslinks("opengl32")
        
    elseif is_os("macosx") then
        add_frameworks("OpenGL", "Cocoa", "IOKit", "CoreVideo")
    elseif is_os("linux") then
        add_syslinks("pthread", "dl")
    end
