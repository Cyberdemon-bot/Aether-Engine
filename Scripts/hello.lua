-- Scripts/hello.lua

function run()
    local sum = 0
    
    -- Vòng lặp tính toán để test độ ổn định trên chip M4
    for i = 1, 10000 do
        sum = sum + (math.sin(i) * math.cos(i))
    end
    
    return sum
end