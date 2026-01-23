set_project("AetherWorkspace")
set_version("1.0.0")

add_rules("mode.debug", "mode.release")
set_languages("c++17")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})

includes("Aether")

target("Sandbox")
    set_kind("binary") 
    set_rundir("$(projectdir)") 
    
    add_files("Sandbox/src/**.cpp")

    add_deps("Aether")

    if is_plat("mingw") then
        add_syslinks("pthread") 
    end

    if is_os("windows") then
        add_syslinks("opengl32")
        add_cxflags("/utf-8")
    elseif is_os("macosx") then
        add_frameworks("OpenGL", "Cocoa", "IOKit", "CoreVideo")
    elseif is_os("linux") then
        add_syslinks("pthread", "dl")
    end
