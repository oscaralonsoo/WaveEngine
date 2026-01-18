public = {
    moveSpeed = 5.0,
    rotationSpeed = 90.0,
    fireRate = 0.5,
    bulletSpeed = 20.0,
    bulletLifetime = 3.0,
    barrelOffset = 3.0  -- Distancia del cañón desde el centro
}

local fireCooldown = 0
local bulletPrefabLoaded = false

function Start(self)
    Engine.Log("=== Tank Controller Started ===")
    Engine.Log("Tank: " .. self.gameObject.name)
    
    -- Cargar prefab de bala
    local loaded = Prefab.Load("Bullet", "Assets/Prefabs/Bullet.prefab")
    if loaded then
        bulletPrefabLoaded = true
        Engine.Log("✓ Bullet prefab loaded successfully")
    else
        Engine.Log("✗ WARNING: Bullet prefab not found!")
        Engine.Log("  Create a bullet GameObject and save it as prefab:")
        Engine.Log("  Right-click on bullet → Save as Prefab → Assets/Prefabs/Bullet.prefab")
    end
end

function Update(self, dt)
    -- Movimiento WASD
    local pos = self.transform.position
    local rot = self.transform.rotation
    local moved = false
    
    -- Calcular dirección forward del tanque
    local radians = math.rad(rot.y)
    local forwardX = math.sin(radians)
    local forwardZ = math.cos(radians)
    
    -- Movimiento hacia adelante/atrás
    if Input.GetKey("W") then
        self.transform:SetPosition(
            pos.x + forwardX * self.public.moveSpeed * dt,
            pos.y,
            pos.z + forwardZ * self.public.moveSpeed * dt
        )
        moved = true
    end
    
    if Input.GetKey("S") then
        self.transform:SetPosition(
            pos.x - forwardX * self.public.moveSpeed * dt,
            pos.y,
            pos.z - forwardZ * self.public.moveSpeed * dt
        )
        moved = true
    end
    
    -- Rotación Q/E
    if Input.GetKey("Q") then
        self.transform:SetRotation(rot.x, rot.y - self.public.rotationSpeed * dt, rot.z)
    end
    
    if Input.GetKey("E") then
        self.transform:SetRotation(rot.x, rot.y + self.public.rotationSpeed * dt, rot.z)
    end
    
    -- Sistema de disparo
    fireCooldown = fireCooldown - dt
    
    if Input.GetKeyDown("Space") and fireCooldown <= 0 then
        if bulletPrefabLoaded then
            FireBullet(self)
            fireCooldown = self.public.fireRate
            Engine.Log("💥 BANG!")
        else
            Engine.Log("Cannot fire: Bullet prefab not loaded")
        end
    end
end

function FireBullet(self)
    -- Instanciar bala desde prefab
    local bullet = Prefab.Instantiate("Bullet")
    
    if bullet == nil then
        Engine.Log("ERROR: Failed to instantiate bullet")
        return
    end
    
    -- Obtener posición y rotación del tanque
    local tankPos = self.transform.position
    local tankRot = self.transform.rotation
    
    -- Calcular posición en la punta del cañón
    local radians = math.rad(tankRot.y)
    local forwardX = math.sin(radians)
    local forwardZ = math.cos(radians)
    
    local spawnX = tankPos.x + forwardX * self.public.barrelOffset
    local spawnY = tankPos.y + 0.5  -- Altura del cañón
    local spawnZ = tankPos.z + forwardZ * self.public.barrelOffset
    
    -- Posicionar bala
    bullet.transform:SetPosition(spawnX, spawnY, spawnZ)
    bullet.transform:SetRotation(tankRot.x, tankRot.y, tankRot.z)
    
    -- La bala YA tiene su script desde el prefab, no hace falta añadirlo
    -- Pero podemos pasarle valores
    -- TODO: Implementar sistema de parámetros entre scripts
end