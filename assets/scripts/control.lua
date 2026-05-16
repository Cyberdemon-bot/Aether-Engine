_speed = 0.0
_time = 0.0
_script = 0

function OnStart()
    _speed = 5.0
    event:Listen("hello", function()
        print("hearded" .. _time)
    end)
end

function OnUpdate(ts)
    local move = Math.Vec3(0.0, 0.0, 0.0)
    _time = _time + ts

    if Input.IsKeyPressed(Key.Up) then
        move = move + Math.Vec3(0.0, 0.0, -1.0)
        physics:AddForce(Math.Vec3(0.0, 100000.0, 0.0))
        event:Fire("test func", 10)
        Native.Async("PrintTest", {}, function() end)
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
    --self.Transform.Translation = current + move * _speed * ts
end

function OnCollision(data)
    print("hello i got the signal!")
    print(data.EntityId)
    print(CollisionType.Name[data.Type])
    print(data.ContactPoint.x, data.ContactPoint.y, data.ContactPoint.z)
end

function OnDestroy()
end