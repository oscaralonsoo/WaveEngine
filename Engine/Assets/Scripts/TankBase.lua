-- TANK BASE ROTATION CONTROLLER
-- Solo maneja la rotación visual del tanque
-- Se aplica a Base.001

public = {
    rotationSpeed = 90.0
}

function Start(self) 
    Engine.Log("=== Tank Base Rotation Controller Started ===")
    Engine.Log("Controls:")
    Engine.Log("  A/D = Rotate Left/Right")
end

function Update(self, dt)
    local rot = self.transform.rotation
    
    if rot == nil then
        Engine.Log("ERROR: Rotation is nil")

        return
    end
 
    -- Obtener velocidad de rotación
    local rotationSpeed = self.public and self.public.rotationSpeed or 90.0
    
    -- Rotación (A/D)
    if Input.GetKey("D") then
        self.transform:SetRotation(rot.x, rot.y - rotationSpeed * dt, rot.z)
    end
    
    if Input.GetKey("A") then
        self.transform:SetRotation(rot.x, rot.y + rotationSpeed * dt, rot.z)
    end
end






