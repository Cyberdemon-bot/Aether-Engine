function OnStart()
    self:Expose("SayHello", function()
        print("Hello from Script B!")
    end)
end

function OnUpdate(ts)
    if Input.IsKeyPressed(Key.E) then
        event:Fire("hello")
    end
end

function OnDestroy()
end