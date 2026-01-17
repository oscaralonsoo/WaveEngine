-- TestScript.lua - Mover GameObject con WASD

public = {
	speed = 5.0
}

function Start(self)
    Engine.Log("Script started on: " .. self.gameObject.name)
    local pos = self.transform.position
    Engine.Log("Initial position: " .. pos.x .. ", " .. pos.y .. ", " .. pos.z)
    Engine.Log("My speed: " .. self.public.speed)
end

function Update(self, dt)
    local moved = false
    local pos = self.transform.position
    local newX = pos.x
    local newY = pos.y
    local newZ = pos.z
    
    -- Mover con WASD
    if Input.GetKey("W") then
        newZ = newZ - self.public.speed * dt
        moved = true
    end
    
    if Input.GetKey("S") then
        newZ = newZ + self.public.speed * dt
        moved = true
    end
    
    if Input.GetKey("A") then
        newX = newX - self.public.speed * dt
        moved = true
    end
    
    if Input.GetKey("D") then
        newX = newX + self.public.speed * dt
        moved = true
        Engine.Log("Moving right at speed: " .. self.public.speed)
    end
    
    -- Aplicar nueva posición si se movió
    if moved then
        self.transform:SetPosition(newX, newY, newZ)
    end
end