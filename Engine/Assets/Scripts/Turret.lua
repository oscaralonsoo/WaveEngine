public = {
    fireRate = 0.5,
    barrelLength = 3.0,
    spawnHeight = 1.8,
    bulletScale = 6.0,
    bulletPrefab = "Bullet.prefab"
}

local fireCooldown = 0
local tankObject = nil
local baseRotX = 0
local baseRotZ = 0
local initialized = false

function Start(self)
    tankObject = GameObject.Find("Tank")
    if not tankObject then
        Engine.Log("[Turret] WARNING: Tank not found!")
    end

    if self.transform and self.transform.rotation then
        baseRotX = self.transform.rotation.x or 0
        baseRotZ = self.transform.rotation.z or 0
    else
        Engine.Log("[Turret] WARNING: Cannot read initial rotation")
    end

    initialized = true
end

function Update(self, dt)
    if not initialized then
        tankObject = GameObject.Find("Tank")
        if self.transform and self.transform.rotation then
            baseRotX = self.transform.rotation.x or 0
            baseRotZ = self.transform.rotation.z or 0
        end
        initialized = true
    end

    if not self or not self.transform then
        Engine.Log("[Turret] ERROR: No transform")
        return
    end

    local mouseX, mouseY = Input.GetMousePosition()
    if not mouseX then
        fireCooldown = fireCooldown - dt
        return
    end

    local worldX, worldZ = Camera.GetScreenToWorldPlane(mouseX, mouseY, 0.0)
    if not worldX then
        fireCooldown = fireCooldown - dt
        return
    end

    local worldPos = self.transform.worldPosition
    if not worldPos then
        Engine.Log("[Turret] ERROR: worldPosition is nil")
        return
    end
    local turretX = worldPos.x
    local turretY = worldPos.y
    local turretZ = worldPos.z

    local dx = worldX - turretX
    local dz = worldZ - turretZ
    local worldAngle = math.deg(math.atan(dx, dz))

    self.transform:SetRotation(baseRotX, worldAngle, baseRotZ)

    fireCooldown = fireCooldown - dt
    if Input.GetKeyDown("Space") and fireCooldown <= 0 then
        local radians = math.rad(worldAngle)
        local forwardX = math.sin(radians)
        local forwardZ = math.cos(radians)

        local barrelLength = self.public and self.public.barrelLength or 3.0
        local spawnHeight = self.public and self.public.spawnHeight or 1.0
        local bulletScale = self.public and self.public.bulletScale or 1.0

        _G.nextBulletData = {
            x = turretX + forwardX * barrelLength,
            y = turretY + spawnHeight,
            z = turretZ + forwardZ * barrelLength,
            dirX = forwardX,
            dirZ = forwardZ,
            angle = worldAngle,
            scale = bulletScale
        }

        Prefab.Instantiate(self.public and self.public.bulletPrefab or "Bullet.prefab")
        fireCooldown = self.public and self.public.fireRate or 0.5
    end
end
