function OnStart()
   
end

function OnUpdate(ts)
    if Input.IsKeyPressed(Key.K) then
        Event:Fire("bruh")
    end
end

function OnDestroy()
end