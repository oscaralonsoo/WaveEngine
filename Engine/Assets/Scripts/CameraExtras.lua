

public = {
    zoomSpeed      = 3.0,
    zoomMin        = 2.0,
    zoomMax        = 15.0,
    shakeDuration  = 0.4,
    shakeMagnitude = 0.3,
    shakeFrequency = 25.0,
}


local initialized  = false
local shakeTimer   = 0.0
local shakeOffsetX = 0.0   -- desplazamiento en X local
local shakeOffsetZ = 0.0   -- desplazamiento en Z local (cenital: eje del suelo)
local baseX, baseY, baseZ = 0.0, 0.0, 0.0



local function clamp(v, min, max)
    if v < min then return min end
    if v > max then return max end
    return v
end



local function Init(self)
    local pos = self.transform.position
    baseX, baseY, baseZ = pos.x, pos.y, pos.z
    initialized = true
    Engine.Log("------------ CameraFeatures: Init --------------------------------------------")
    Engine.Log(string.format("[CameraFeatures] Pos local inicial: %.2f, %.2f, %.2f", baseX, baseY, baseZ))
    Engine.Log("  5 = Zoom In  (mantener)")
    Engine.Log("  6 = Zoom Out (mantener)")
    Engine.Log("  7 = Camera Shake")
end



local function UpdateZoom(self, dt)
    local zoomDir = 0
    if Input.GetKey("5") then zoomDir = -1 end  -- acercar
    if Input.GetKey("6") then zoomDir =  1 end  -- alejar
    if zoomDir == 0 then return end

    local pos = self.transform.position
    local cfg = self.public

    
    local dist = math.sqrt(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z)
    if dist < 0.001 then return end

  
    local newDist = clamp(dist + zoomDir * cfg.zoomSpeed * dt, cfg.zoomMin, cfg.zoomMax)

    
    local scale = newDist / dist
    baseX = pos.x * scale
    baseY = pos.y * scale
    baseZ = pos.z * scale

    self.transform:SetPosition(baseX + shakeOffsetX, baseY, baseZ + shakeOffsetZ)
end



local function TriggerShake(self)
    local pos = self.transform.position
    baseX = pos.x - shakeOffsetX
    baseY = pos.y
    baseZ = pos.z - shakeOffsetZ
    shakeTimer = self.public.shakeDuration
    Engine.Log("[CameraFeatures] Shake triggered")
end

local function UpdateShake(self, dt)
    local cfg = self.public

    if shakeTimer <= 0.0 then
        if shakeOffsetX ~= 0.0 or shakeOffsetZ ~= 0.0 then
            shakeOffsetX = 0.0
            shakeOffsetZ = 0.0
            self.transform:SetPosition(baseX, baseY, baseZ)
        end
        return
    end

    shakeTimer = shakeTimer - dt

    local progress  = shakeTimer / cfg.shakeDuration
    local amplitude = cfg.shakeMagnitude * progress
    local t         = cfg.shakeDuration - shakeTimer

    -- Shake en X y Z (ejes del suelo, visibles desde camara cenital)
    shakeOffsetX = amplitude * math.sin(t * cfg.shakeFrequency * 2.0 * math.pi)
    shakeOffsetZ = amplitude * math.sin(t * cfg.shakeFrequency * 2.0 * math.pi * 1.3)

    self.transform:SetPosition(baseX + shakeOffsetX, baseY, baseZ + shakeOffsetZ)

    if shakeTimer <= 0.0 then
        shakeOffsetX = 0.0
        shakeOffsetZ = 0.0
        self.transform:SetPosition(baseX, baseY, baseZ)
        Engine.Log("[CameraFeatures] Shake finished")
    end
end



function Start(self)
    Init(self)
end

function Update(self, dt)
    if not initialized then
        Init(self)
    end

    UpdateZoom(self, dt)

    if Input.GetKeyDown("7") then
        TriggerShake(self)
    end

    UpdateShake(self, dt)
end