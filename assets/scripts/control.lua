_speed = 0.0
_time = 0.0
_script = 0

function OnStart()
    _speed = 5.0
    Coroutine:async(function()
        print("Step 1 - starting")
        
        Coroutine:await()
        print("Step 2 - resumed after one frame")
        
        Coroutine:await(Coroutine:Sleep(3))
        print("Step 3 - resumed after 3 seconds")
        
        print("Done!")
    end)
end

function OnUpdate(ts)
    local move = Math.Vec3(0.0, 0.0, 0.0)
    _time = _time + ts

    if Input.IsKeyPressed(Key.Up) then
        move = move + Math.Vec3(0.0, 0.0, -1.0)
        Physics:AddForce(Math.Vec3(0.0, 100000.0, 0.0))
        Native.PrintTest()
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
    print(data.EntityId)
    print(CollisionType.Name[data.Type])
    print(data.ContactPoint.x, data.ContactPoint.y, data.ContactPoint.z)
end

function OnDestroy()
end