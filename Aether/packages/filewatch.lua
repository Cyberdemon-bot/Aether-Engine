package("filewatch")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/ThomasMonkman/filewatch")
    set_description("C++11 cross-platform file watching")
    set_urls("https://github.com/ThomasMonkman/filewatch.git")

    on_install(function (package)
        os.cp("FileWatch.hpp", package:installdir("include"))
    end)