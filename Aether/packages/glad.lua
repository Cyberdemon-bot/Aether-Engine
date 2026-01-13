package("glad")
    set_kind("library", {headeronly = false})
    set_description("Local Glad OpenGL loader")

    on_install(function (package)
        import("package.tools.xmake")

        local project_dir = os.workingdir()
        local local_glad_src = path.join(project_dir, "Aether/vendor/glad/src/glad.c")
        local local_glad_inc = path.join(project_dir, "Aether/vendor/glad/include")

        if not os.isfile(local_glad_src) then
            raise("GLAD ERROR: Cannot find out files at " .. local_glad_src)
        end

        os.cp(local_glad_src, "src/glad.c")
        os.cp(local_glad_inc, "include")
        os.cp(path.join(local_glad_inc, "*"), package:installdir("include"))

        local xmake_lua = [[
            target("glad")
                set_kind("static")
                add_files("src/glad.c")
                add_includedirs("include") 
                add_includedirs("include", {public = true})
        ]]
        io.writefile("xmake.lua", xmake_lua)

        -- 4. Build lib
        xmake.install(package)
    end)