_speed = 0.0
_time = 0.0
_script = 0

function OnStart()
    _speed = 5.0
    event:Listen("hello", function()
        print("hearded" .. _time)
    end)
    _script = scene:LoadScript("assets/scripts/test.lua")
end

function OnUpdate(ts)
    local move = Math.Vec3(0.0, 0.0, 0.0)
    _time = _time + ts

    if Input.IsKeyPressed(Key.Up) then
        move = move + Math.Vec3(0.0, 0.0, -1.0)
    end
    if Input.IsKeyPressed(Key.Down) then
        move = move + Math.Vec3(0.0, 0.0, 1.0)
    end
    if Input.IsKeyPressed(Key.Left) then
        move = move + Math.Vec3(-1.0, 0.0, 0.0)
    end
    if Input.IsKeyPressed(Key.Right) then
        move = move + Math.Vec3(1.0, 0.0, 0.0)
    end
    if Input.IsKeyPressed(Key.Space) then
        move = move + Math.Vec3(0.0, 1.0, 0.0)
    end
    if Input.IsKeyPressed(Key.LeftShift) then
        move = move + Math.Vec3(0.0, -1.0, 0.0)
    end

    local current = self.Transform.Translation
    self.Transform.Translation = current + move * _speed * ts
end

function OnCollision(data)
    print("hello i got the signal!")
end

function OnDestroy()
end