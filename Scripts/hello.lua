-- Scripts/hello.lua

-- Gọi hàm SystemLog của C++
Log("Xin chao tu Lua! Chuan bi test Class C++...")

-- Khởi tạo Object C++ từ Lua
local p1 = Player.new("Arthur", 0.0, 0.0)

Log("Ten nhan vat la: " .. p1.name)

-- Gọi Method C++ để thay đổi logic
p1:Move(10.5, 5.0)
p1:Move(0.0, -2.5)

-- Đọc và ghi đè property C++ trực tiếp
p1.x = 999.9
Log("Toa do X da bi ghi de thanh: " .. tostring(p1.x))

-- Trả về một giá trị cho C++ bắt lấy (giống hàm run() cũ)
return p1.x + p1.y