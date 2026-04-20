#include "TestLayer.h"
#include <iostream>
#include <string>
#include <sol/sol.hpp>

void SystemLog(const std::string& msg) {
    std::cout << "[Engine Log] " << msg << "\n";
}
// =======================================================

TestLayer::TestLayer()
    : Layer("Main Game")
{
   
}

void TestLayer::Attach()
{
    sol::state lua;

    lua.open_libraries(sol::lib::base, sol::lib::math);

    lua.script(R"(
        if jit then
            print("Running on LuaJIT:", jit.version)
        else
            print("Not LuaJIT")
        end
    )");

    // benchmark đơn giản
    lua.script(R"(
        local t = 0
        for i = 1, 10000000 do
            t = t + i
        end
        print("Result:", t)
    )");
}

void TestLayer::Detach() 
{
}

void TestLayer::Update(Aether::Timestep ts)
{
}

void TestLayer::OnImGuiRender() 
{
}

void TestLayer::OnEvent(Aether::Event& event)
{
}