set_project("AetherWorkspace")
set_version("1.0.0")

add_rules("mode.debug", "mode.release")
set_languages("c++17")

includes("Aether")

target("Sandbox")
    set_kind("binary") 
    set_rundir("$(projectdir)") 
    
    add_files("Sandbox/src/**.cpp")

    add_deps("Aether")