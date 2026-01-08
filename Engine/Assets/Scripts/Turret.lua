
speed = 0.01;
bullets = {}

function Start()
    obj = FindGameObject("this")
end

function Update()

    if Input.MouseRight  and not lastMouseLeft  then    
        bullet = CreatePrimitive("Cube", "bullet")
        table.insert(bullets, bullet)
        SetPosition(bullets[#bullets],Input.MouseX/100,0,Input.MouseY/100)  
    end   

    lastMouseLeft = Input.MouseRight        

    pos = GetPosition(obj) 
    dx = Input.MouseX - (pos.x * 100)
    dy = Input.MouseY - (pos.y * 100)
    angle  = atan2(dy/dx) * -1
    angleDeg = math.deg(angle)
    SetRotation(obj,0,angleDeg,0)

end

function atan2(y, x)
    if x > 0 then
        return math.atan(y / x)
    elseif x < 0 then
        if y >= 0 then
            return math.atan(y / x) + math.pi
        else
            return math.atan(y / x) - math.pi
        end
    elseif y > 0 then
        return math.pi / 2
    elseif y < 0 then
        return -math.pi / 2
    else
        return 0
    end
end
