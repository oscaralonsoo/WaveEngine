-- TANK CONTROLLER 
-- Controles:
-- W/S: Mover adelante/atrás (en la dirección que mira)
-- A/D: Girar el tanque izquierda/derecha

public = {
    moveSpeed = 5.0,
    rotationSpeed = 90.0
}

function Start(self)
    Engine.Log("=== Tank Movement Controller Started ===")
    Engine.Log("Tank: " .. self.gameObject.name)
    Engine.Log("Controls:")
    Engine.Log("  W/S = Forward/Backward")
    Engine.Log("  A/D = Rotate Left/Right")
end

function Update(self, dt)
    local pos = self.transform.position
    local rot = self.transform.rotation
    
    if pos == nil or rot == nil then
        Engine.Log("ERROR: Transform data is nil")
        return
    end
    
    -- Obtener velocidades desde variables pubnli
    local moveSpeed = self.public and self.public.moveSpeed or 5.0
    local rotationSpeed = self.public and self.public.rotationSpeed or 90.0
    
    -- Calcular vector forward del tanque
    local tankRadians = math.rad(rot.y)
    local forwardX = math.sin(tankRadians)
    local forwardZ = math.cos(tankRadians)
    
    -- Movimiento adelante/atrás (W/S)
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
    
    -- Rotación del tanque (A/D)
    if Input.GetKey("A") then
        self.transform:SetRotation(rot.x, rot.y - rotationSpeed * dt, rot.z)
    end
    
    if Input.GetKey("D") then
        self.transform:SetRotation(rot.x, rot.y + rotationSpeed * dt, rot.z)
    end
end