function OnStart()
    self._speed = 5.0
end

function OnUpdate(ts)
    local move = Math.Vec3(0.0, 0.0, 0.0)

    if Input.IsKeyPressed(Key.KeyCode.Up) then
        move = move + Math.Vec3(0.0, 0.0, -1.0)
    end
    if Input.IsKeyPressed(Key.KeyCode.Down) then
        move = move + Math.Vec3(0.0, 0.0, 1.0)
    end
    if Input.IsKeyPressed(Key.KeyCode.Left) then
        move = move + Math.Vec3(-1.0, 0.0, 0.0)
    end
    if Input.IsKeyPressed(Key.KeyCode.Right) then
        move = move + Math.Vec3(1.0, 0.0, 0.0)
    end
    if Input.IsKeyPressed(Key.KeyCode.Space) then
        move = move + Math.Vec3(0.0, 1.0, 0.0)
    end
    if Input.IsKeyPressed(Key.KeyCode.LeftShift) then
        move = move + Math.Vec3(0.0, -1.0, 0.0)
    end

    local current = self.Transform.Translation
    self.Transform.Translation = current + move * self._speed * ts
end

function OnDestroy()
end