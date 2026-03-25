#include "TestLayer.h"
#include <iostream>
#include <string>

// Include thư viện sol2 thay cho Lua C API thủ công
#include <sol/sol.hpp>
#include "Aether/Scripting/Math.h"

// =======================================================
// CÁC THÀNH PHẦN C++ DÙNG ĐỂ THỬ NGHIỆM BINDING SANG LUA
// =======================================================
struct TestPlayer {
    std::string name;
    float x, y;

    TestPlayer(const std::string& n, float startX, float startY) 
        : name(n), x(startX), y(startY) {}

    void Move(float dx, float dy) {
        x += dx;
        y += dy;
        std::cout << "[C++] " << name << " vua di chuyen den toa do (" << x << ", " << y << ")\n";
    }
};

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
    std::cout << "--- BAT DAU THU NGHIEM SOL2 ---\n";

    // 1. Khởi tạo Lua State thông qua sol2 (Thay cho luaL_newstate)
    // Lưu ý: Biến 'lua' dùng RAII, nó sẽ tự động dọn dẹp và đóng máy ảo khi thoát hàm Attach
    sol::state lua;
    
    // Mở các thư viện tiêu chuẩn cần thiết
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

    // 2. Bind (Ràng buộc) hàm tự do của C++
    lua["Log"] = SystemLog;

    // 3. Bind toàn bộ Class C++
    lua.new_usertype<TestPlayer>("Player",
        sol::constructors<TestPlayer(const std::string&, float, float)>(),
        "name", &TestPlayer::name,
        "x", &TestPlayer::x,
        "y", &TestPlayer::y,
        "Move", &TestPlayer::Move
    );

    // 4. Load và chạy file script ngoài an toàn (thay cho luaL_dofile)
    try 
    {
        // script_file sẽ tự động load, compile và chạy file hello.lua
        auto result = lua.script_file("Scripts/hello.lua");
        
        // Trích xuất kết quả trả về từ file Lua (nếu có)
        double return_val = result;
        std::cout << "Returned from Lua: " << return_val << "\n";
    }
    catch (const sol::error& e) 
    {
        // Xử lý lỗi (sai cú pháp, gọi sai hàm, v.v.)
        std::cerr << "Lua script execution error:\n" << e.what() << std::endl;
    }

    std::cout << "--- KET THUC THU NGHIEM ---\n";
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