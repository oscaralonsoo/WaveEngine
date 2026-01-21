-- BULLET CONTROLLER - SIMPLE PROJECTILE

public = {
    speed = 20.0,
    lifetime = 5.0
}

local timeAlive = 0
local direction = {x = 0, y = 0, z = 1}  -- Default forward
local initialized = false
local hasMoved = false  -- Track if we've set initial position

function Start(self)
    Engine.Log("=== Bullet Spawned ===")
end

function Update(self, dt)
    -- Initialize on first Update (spawn data is ready now)
    if not initialized then
        if _G.nextBulletData then
            local data = _G.nextBulletData
            
            -- Set initial position
            self.transform:SetPosition(data.x, data.y, data.z)
            
            -- Use pre-calculated direction from turret
            direction.x = data.dirX
            direction.y = 0
            direction.z = data.dirZ
            
            initialized = true
            
            Engine.Log(string.format(" Bullet initialized | Angle: %.1f° | Dir: (%.3f, 0, %.3f) | Pos: (%.1f, %.1f, %.1f)", 
                data.angle, direction.x, direction.z, data.x, data.y, data.z))
            
            -- Clear the global data for next bullet
            _G.nextBulletData = nil
            
            -- Don't move this frame, just set position
            return
        else
            Engine.Log(" WARNING: Bullet has no spawn data - using defaults")
            initialized = true
        end
    end
    
    local pos = self.transform.position
    
    if pos == nil then
        Engine.Log("ERROR: Bullet position is nil")
        return
    end
    
    -- Get speed and lifetime from public variables
    local speed = self.public and self.public.speed or 20.0
    local lifetime = self.public and self.public.lifetime or 5.0
    
    -- Move bullet in calculated direction
    local newX = pos.x + direction.x * speed * dt
    local newY = pos.y + direction.y * speed * dt
    local newZ = pos.z + direction.z * speed * dt
    
    self.transform:SetPosition(newX, newY, newZ)
    
    -- Lifetime control
    timeAlive = timeAlive + dt
    
    if timeAlive >= lifetime then
        Engine.Log(string.format(" Bullet destroyed after %.2f seconds", timeAlive))
        self:Destroy()
    end
end