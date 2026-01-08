
x = 0.0;
y = 0.0;
rot = 0.0;

function Start()
    obj = FindGameObject("this")
    if obj == nil then
        print("ERROR: No se ha encontrado el objeto!")
    else
        print("Objeto encontrado correctamente")
    end	

    print("Hello from Lua Start")
end

function Update()

    if Input.W then
        SetPosition(obj,x, 0, y)

        x = x + 0.1

    end

    if Input.S then
        SetPosition(obj,x, 0, y)
        x = x - 0.1
    end

    if Input.D then
        SetPosition(obj,x, 0, y)
        y = y - 0.1
    end
    if Input.A then
        SetPosition(obj,x, 0, y)
        y = y + 0.1
    end

    if Input.MouseLeft then
        print(Input.MouseX)
        print(Input.MouseY) 

    end
    if Input.MouseRight then    
        print(angleDeg)
        print(GetPosition(obj))


    end   
    pos = GetPosition(obj) 
    dx = Input.MouseX - (pos.x * 100)
    dy = Input.MouseY - (pos.y * 100)
    angle  = atan2(dy,dx) * -1
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
