
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
    Engine.Log("[Turret] ===== Start() called =====")
    
    -- Buscar el tanque
    tankObject = GameObject.Find("Tank")
    if tankObject then
        Engine.Log("[Turret] Tank found: " .. tankObject.name)
    else
        Engine.Log("[Turret] WARNING: Tank not found!")
    end
    
    -- Intentar guardar rotación base
    if self.transform and self.transform.rotation then
        baseRotX = self.transform.rotation.x or 0
        baseRotZ = self.transform.rotation.z or 0
        Engine.Log(string.format("[Turret] Base rotation saved: X=%.2f, Z=%.2f", baseRotX, baseRotZ))
    else
        Engine.Log("[Turret] WARNING: Cannot read initial rotation")
    end
    
    initialized = true
end

function Update(self, dt)
    -- Inicializar en Update si Start no se llamó
    if not initialized then
        Engine.Log("[Turret] Initializing in Update (Start not called yet)")
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
    
    -- Obtener posición del mouse
    local mouseX, mouseY = Input.GetMousePosition()
    if not mouseX then 
        fireCooldown = fireCooldown - dt
        return 
    end
    
    -- Convertir a coordenadas del mundo
    local worldX, worldZ = Camera.GetScreenToWorldPlane(mouseX, mouseY, 0.0)
    if not worldX then 
        fireCooldown = fireCooldown - dt
        return 
    end
    
    -- Calcular posición mundial de la torreta
    local turretX, turretY, turretZ = 0, 0, 0
    
    if tankObject and tankObject.transform then
        local tankPos = tankObject.transform.position
        local tankRot = tankObject.transform.rotation
        
        if tankPos and tankRot and self.transform.position then
            local localPos = self.transform.position
            
            local tankRadians = math.rad(tankRot.y)
            turretX = tankPos.x + localPos.x * math.cos(tankRadians) - localPos.z * math.sin(tankRadians)
            turretY = tankPos.y + localPos.y
            turretZ = tankPos.z + localPos.x * math.sin(tankRadians) + localPos.z * math.cos(tankRadians)
        end
    else
        if self.transform.position then
            turretX = self.transform.position.x
            turretY = self.transform.position.y
            turretZ = self.transform.position.z
        end
    end
    
    -- Calcular ángulo hacia el mouse
    local dx = worldX - turretX
    local dz = worldZ - turretZ
    local worldAngle = math.deg(math.atan(dx, dz))
    
    -- Restar rotación del tanque
    local tankYaw = 0
    if tankObject and tankObject.transform and tankObject.transform.rotation then
        tankYaw = tankObject.transform.rotation.y or 0
    end
    
    local localYaw = worldAngle - tankYaw
    
    -- Log ocasional para debug
    if math.random(1, 120) == 1 then
        Engine.Log(string.format("[Turret] Rotating to: X=%.1f, Y=%.1f, Z=%.1f", baseRotX, localYaw, baseRotZ))
    end
    
    self.transform:SetRotation(baseRotX, localYaw, baseRotZ)
    
    -- Sistema de disparo
    fireCooldown = fireCooldown - dt
    if Input.GetKeyDown("Space") and fireCooldown <= 0 then
        local radians = math.rad(worldAngle)
        local forwardX = math.sin(radians)
        local forwardZ = math.cos(radians)
        
        local barrelLength = self.public and self.public.barrelLength or 3.0
        local spawnHeight = self.public and self.public.spawnHeight or 1.0
        local bulletScale = self.public and self.public.bulletScale or 1.0
        local spawnX = turretPos.x + forwardX * barrelLength
        local spawnY = turretPos.y + spawnHeight
        local spawnZ = turretPos.z + forwardZ * barrelLength

        _G.nextBulletData = {
            x = spawnX,
            y = spawnY,
            z = spawnZ,
            dirX = forwardX,
            dirZ = forwardZ,
            angle = worldAngle
        }
        
        Prefab.Instantiate(self.public and self.public.bulletPrefab or "Bullet.prefab")
        fireCooldown = self.public and self.public.fireRate or 0.5
        
        Engine.Log(string.format("[Turret] FIRED! Angle: %.1f°", worldAngle))
    end
end