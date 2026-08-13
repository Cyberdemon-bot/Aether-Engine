set_project("AetherWorkspace")
set_version("1.0.0")

set_languages("c++20")
add_rules("mode.debug", "mode.release")
set_targetdir("$(projectdir)/bin/$(mode)")
includes("Aether")

includes("Game")
includes("Sandbox")