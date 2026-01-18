public = {
    speed = 20.0,
    lifetime = 5.0
}

local timeAlive = 0
local direction = {x = 0, y = 0, z = 0}

function Start(self)
    Engine.Log("Bullet spawned: " .. self.gameObject.name)
    
    -- Calcular dirección basada en la rotación inicial
    local rot = self.transform.rotation
    local radians = math.rad(rot.y)
    
    direction.x = math.sin(radians)
    direction.y = 0
    direction.z = math.cos(radians)
    
    Engine.Log("Bullet direction: " .. direction.x .. ", " .. direction.z)
end

function Update(self, dt)
    -- Mover hacia adelante en su dirección
    local pos = self.transform.position
    
    self.transform:SetPosition(
        pos.x + direction.x * self.public.speed * dt,
        pos.y + direction.y * self.public.speed * dt,
        pos.z + direction.z * self.public.speed * dt
    )
    
    -- Autodestruir después del tiempo de vida
    timeAlive = timeAlive + dt
    
    if timeAlive >= self.public.lifetime then
        Engine.Log("Bullet destroyed (lifetime: " .. timeAlive .. "s)")
        GameObject.Destroy(self.gameObject)
    end
end

-- ==========================================
-- CÓMO CREAR EL PREFAB DE BALA:
-- ==========================================
--[[
1. En tu motor, crea un GameObject llamado "Bullet"
2. Añádele componentes:
   - ComponentMesh: Usa una esfera pequeña (primitiva)
     O importa un modelo de bala .fbx
   - ComponentMaterial: Dale un color rojo/amarillo
   - ComponentScript: Añade "Assets/Scripts/Bullet.lua"
3. Ajusta la escala (ej: 0.2, 0.2, 0.2) para que sea pequeña
4. En el Hierarchy, click derecho en "Bullet"
5. Selecciona "Save as Prefab..."
6. Guárdalo en: Assets/Prefabs/Bullet.prefab
7. Elimina el objeto "Bullet" de la escena
8. ¡Listo! El tanque ahora puede disparar balas
]]