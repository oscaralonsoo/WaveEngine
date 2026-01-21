-- TURRET CONTROLLER - MOUSE AIMING (HORIZONTAL ONLY)
-- GameObject structure: Tank > Barrel (this script)

public = {
    fireRate = 0.5,
    barrelLength = 3.0,
    groundPlaneY = 0.0,
    bulletPrefab = "Bullet.prefab",
    basePitchOffset = -90.0
}

local fireCooldown = 0
local currentYaw = 0

function Start(self)
    Engine.Log("=== Turret Controller Started ===")
    Engine.Log("Turret (Barrel): " .. self.gameObject.name)
    Engine.Log("Controls: MOUSE = Aim | SPACE = Shoot")
    
    local pitchOffset = self.public and self.public.basePitchOffset or 0.0
    self.transform:SetRotation(pitchOffset, 0, 0)
    
    currentYaw = 0
end

function Update(self, dt)
    -- === MOUSE AIMING ===
    local mouseX, mouseY = Input.GetMousePosition()
    
    if mouseX == nil or mouseY == nil then
        return
    end
    
    local groundPlaneY = self.public and self.public.groundPlaneY or 0.0
    local worldX, worldZ = Camera.GetScreenToWorldPlane(mouseX, mouseY, groundPlaneY)
    
    if worldX == nil or worldZ == nil then
        return
    end
    
    local turretPos = self.transform.position
    
    if turretPos == nil then
        Engine.Log("ERROR: Cannot get turret position")
        return
    end
    
    -- Calculate direction to cursor
    local dx = worldX - turretPos.x
    local dz = worldZ - turretPos.z
    
    local distance = math.sqrt(dx * dx + dz * dz)
    if distance < 0.01 then
        return
    end
    
    -- Calculate angle using math.atan (Lua 5.1/5.2)
    local angleRadians = math.atan(dx, dz)
    local angleDegrees = angleRadians * (180.0 / math.pi)
    
    currentYaw = angleDegrees
    
    -- Apply rotation
    local pitchOffset = self.public and self.public.basePitchOffset or 0.0
    self.transform:SetRotation(pitchOffset, currentYaw, 0)
    
    -- === FIRING SYSTEM ===
    fireCooldown = fireCooldown - dt
    
    if Input.GetKeyDown("Space") and fireCooldown <= 0 then
        FireBullet(self)
        fireCooldown = self.public and self.public.fireRate or 0.5
    end
end

function FireBullet(self)
    local turretPos = self.transform.position
    
    if turretPos == nil then
        Engine.Log("ERROR: Cannot get turret position")
        return
    end
    
    -- Calculate firing direction from currentYaw
    local radians = math.rad(currentYaw)
    local forwardX = math.sin(radians)
    local forwardZ = math.cos(radians)
    
    -- Calculate spawn position at barrel tip
    local barrelLength = self.public and self.public.barrelLength or 3.0
    local spawnX = turretPos.x + forwardX * barrelLength
    local spawnY = turretPos.y + 0.8
    local spawnZ = turretPos.z + forwardZ * barrelLength
    
    -- Store bullet spawn data in global table
    _G.nextBulletData = {
        x = spawnX,
        y = spawnY,
        z = spawnZ,
        angle = currentYaw,
        dirX = forwardX,
        dirZ = forwardZ
    }
    
    Engine.Log(string.format("💥 Bullet fired | Angle: %.1f° | Dir: (%.3f, 0, %.3f) | Spawn: (%.1f, %.1f, %.1f)", 
        currentYaw, forwardX, forwardZ, spawnX, spawnY, spawnZ))
    
    local bulletPrefab = self.public and self.public.bulletPrefab or "Bullet.prefab"
    Prefab.Instantiate(bulletPrefab)
end