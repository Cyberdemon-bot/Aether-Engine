function OnStart()
    self._time = 0.0
end

function OnUpdate(ts)

    self._time = self._time + ts
    local t = self._time

    local axis = Math.Vec3(0.0, 1.0, 0.0):Normalize()
    local spin = Math.FromAxisAngle(axis, t * 1.3)

    local radius = 4.0 + Math.Vec3(math.sin(t * 0.7), math.cos(t * 0.4), 0.0):Length()

    local spoke = spin * Math.Vec3(radius, 0.0, 0.0)
    
    local breathe = math.sin(t * 2.5) * 1.2
    local twistAxis = Math.Vec3(1.0, 0.0, 1.0):Normalize()
    local twist = Math.FromAxisAngle(twistAxis, t * 0.9)
    local wobble = twist * Math.Vec3(0.0, breathe, 0.0)
    
    local final = spoke + wobble
    self.Transform.Translation = final
end

function OnDestroy()
end