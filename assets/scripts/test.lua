function OnStart()
    self:Expose("SayHello", function()
        print("Hello from Script B!")
    end)
end

function OnUpdate(ts)
end

function OnDestroy()
end