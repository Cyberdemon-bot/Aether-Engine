#include "TestLayer.h"
#include <iostream>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

TestLayer::TestLayer()
    : Layer("Main Game")
{
   
}

void TestLayer::Attach()
{
    // 1. Khởi tạo Lua State tiêu chuẩn
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    // 2. Load script
    if (luaL_dofile(L, "Scripts/hello.lua") != LUA_OK)
    {
        std::cerr << "Lua error: "
                  << lua_tostring(L, -1) << std::endl;
        lua_close(L);
        return;
    }

    // 3. Gọi function run()
    lua_getglobal(L, "run");

    if (lua_pcall(L, 0, 1, 0) != LUA_OK)
    {
        std::cerr << "Lua run() error: "
                  << lua_tostring(L, -1) << std::endl;
        lua_close(L);
        return;
    }

    // 4. Lấy kết quả trả về
    double result = lua_tonumber(L, -1);
    std::cout << "Returned from Lua: " << result << std::endl;

    lua_pop(L, 1);

    // 5. Đóng Lua State (Lưu ý xem mục chú ý bên dưới)
    lua_close(L);
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