package("ozz-animation")
    set_homepage("https://github.com/guillaumeblanc/ozz-animation")
    set_description("Open source c++ skeletal animation library and toolset.")

    -- Lấy source từ github
    set_urls("https://github.com/guillaumeblanc/ozz-animation.git")

    -- Dùng CMake để build
    add_deps("cmake")

    on_install(function (package)
        import("package.tools.cmake")
        
        -- Cấu hình CMake options cho Ozz
        local configs = {
            "-DOZZ_BUILD_TOOLS=ON",    -- Quan trọng: Build tool để convert gltf -> ozz
            "-DOZZ_BUILD_SAMPLES=OFF", -- Tắt sample cho nhẹ
            "-DOZZ_BUILD_TESTS=OFF"    -- Tắt test
        }
        
        -- Build static library
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF") 

        cmake.install(package, configs)
    end)
package_end()