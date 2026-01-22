
camx = 0
camz = 0
function Start()
    camera = FindGameObject("this")
    tank = FindGameObject("tank")

    SetPosition(camera,0,10,0)  
    SetRotation(camera,-90,0,0)  
end

function Update()
    posTank = GetPosition(tank)
    SetPosition(camera,posTank.x,10,posTank.z)
end



