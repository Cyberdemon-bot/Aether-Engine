function OnStart()
    self._speed = 5.0
    self._time = 0.0
end

function OnUpdate(ts)
    local move = Math.Vec3(0.0, 0.0, 0.0)
    self._time = self._time + ts

    if Input.IsMouseButtonPressed(Mouse.Button0) then
        print(self._time)
    end

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
    self.Transform.Translation = current + move * self._speed * ts
end

function OnDestroy()
end