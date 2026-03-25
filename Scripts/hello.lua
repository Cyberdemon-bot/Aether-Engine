-- ==========================================
-- SCRIPT: Tương tác cơ bản với Entity
-- ==========================================

-- Được gọi 1 lần duy nhất khi Entity vừa được khởi tạo và nạp script
function OnStart()
    print("Entity đã được sinh ra! Bắt đầu khởi tạo dữ liệu...")
    
    -- Ví dụ: Đặt vị trí ban đầu
    if self.Transform then
        self.Transform.Translation.x = 0.0
        self.Transform.Translation.y = 0.0
        self.Transform.Translation.z = 0.0
    end
end

-- Được gọi liên tục mỗi frame
-- ts (TimeStep / DeltaTime) giúp chuyển động mượt mà không phụ thuộc FPS
function OnUpdate(ts)
    if self.Transform then
        local speed = 5.0
        local t = self.Transform.Translation  -- gets a Vec3 copy
        t.x = t.x + speed * ts
        self.Transform.Translation = t   
        self.Transform.Dirty = true;
    end
end

-- Được gọi 1 lần ngay trước khi Entity bị xóa khỏi Scene
function OnDestroy()
    print("Entity chuẩn bị bị hủy! Đang dọn dẹp...")
end