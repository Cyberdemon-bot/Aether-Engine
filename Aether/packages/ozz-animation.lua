package("ozz-animation")
    set_homepage("https://github.com/guillaumeblanc/ozz-animation")
    set_description("Open source c++ skeletal animation library and toolset.")
    set_urls("https://github.com/guillaumeblanc/ozz-animation.git")
    add_deps("cmake")

    on_install(function (package)
        import("package.tools.cmake")
        
        local configs = {
            "-DOZZ_BUILD_TOOLS=OFF",   
            "-DOZZ_BUILD_SAMPLES=OFF", 
            "-DOZZ_BUILD_TESTS=OFF"   
        }
        
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF") 

        cmake.install(package, configs)
    end)
package_end()