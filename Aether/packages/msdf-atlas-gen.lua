package("msdf-atlas-gen")
    set_homepage("https://github.com/Chlumsky/msdf-atlas-gen")
    set_description("MSDF atlas generator (Native xmake build)")
    set_urls("https://github.com/Chlumsky/msdf-atlas-gen.git")

    add_deps("freetype")

    on_install(function (package)
        io.writefile("msdfgen/msdfgen-config.h", [[
            #pragma once
            #define MSDFGEN_PUBLIC
            #define MSDFGEN_VERSION "1.10.0"
        ]])

        local xmake_script = [[
            add_rules("mode.release", "mode.debug")
            add_requires("freetype")

            target("msdf-atlas-gen")
                set_kind("static")
                set_languages("c++17") 
                add_defines("MSDFGEN_USE_CPP11", "MSDFGEN_EXTENSIONS")
                add_defines("MSDFGEN_DISABLE_SVG") 
                add_includedirs(".", "msdfgen", {public = true})
                add_files("msdf-atlas-gen/*.cpp")
                add_files("msdfgen/core/*.cpp")
                add_files("msdfgen/ext/*.cpp")
                remove_files("msdfgen/ext/import-font.cpp") 
                remove_files("msdf-atlas-gen/artery-font-export.cpp") -- File này cần lib ngoài, bỏ đi cho nhẹ
                remove_files("msdfgen/main.cpp")
                
                add_packages("freetype")
        ]]
        
        io.writefile("xmake.lua", xmake_script)

        -- BƯỚC 3: Build
        import("package.tools.xmake").install(package)
    end)