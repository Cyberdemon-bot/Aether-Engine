function OnStart()
   
end

function OnUpdate(ts)
    if Input.IsKeyPressed(Key.K) then
        Event:Fire("K")
    elseif Input.IsKeyPressed(Key.J) then
        Event:Fire("J")
    elseif Input.IsKeyPressed(Key.L) then
        Event:Fire("L")
    elseif Input.IsKeyPressed(Key.R) then
        Event:Fire("R")
    end
end

function OnDestroy()
end