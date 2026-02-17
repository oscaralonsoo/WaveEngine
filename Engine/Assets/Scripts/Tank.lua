-- TANK MOVEMENT CONTROLLER

public = {
    moveSpeed = 5.0
}

local baseObject = nil

function Start(self)
    baseObject = GameObject.Find("Base.001")
    if not baseObject then
        Engine.Log("[Tank] WARNING: Base.001 not found!")
    end
end

function Update(self, dt)
    local pos = self.transform.position

    if pos == nil then
        Engine.Log("[Tank] ERROR: Position is nil")
        return
    end

    if not baseObject then
        baseObject = GameObject.Find("Base.001")
    end

    local moveSpeed = self.public and self.public.moveSpeed or 5.0

    local visualYaw = 0
    if baseObject and baseObject.transform and baseObject.transform.rotation then
        visualYaw = baseObject.transform.rotation.y or 0
    end

    local radians = math.rad(visualYaw)
    local forwardX = math.sin(radians)
    local forwardZ = math.cos(radians)

    if Input.GetKey("S") then
        self.transform:SetPosition(
            pos.x - forwardX * moveSpeed * dt,
            pos.y,
            pos.z - forwardZ * moveSpeed * dt
        )
    end

    if Input.GetKey("W") then
        self.transform:SetPosition(
            pos.x + forwardX * moveSpeed * dt,
            pos.y,
            pos.z + forwardZ * moveSpeed * dt
        )
    end
end

