set_project("AetherWorkspace")
set_version("1.0.0")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})

includes("Aether")

target("Sandbox")
    set_kind("binary")
    set_languages("c++20")
    set_rundir("$(projectdir)")
    add_files("Sandbox/src/**.cpp")
    add_deps("Aether")
    add_defines("AETHER_SHARED")
    

    if is_mode("debug") then
        set_policy("build.sanitizer.address", true)
    end

    if is_mode("release") then
        --set_symbols("debug")     
        --set_optimize("fastest")   
    end