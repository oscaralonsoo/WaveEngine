
x = 0.0;
function Start()
    
    print("Hello from Lua Start")
end

function Update()
    SetPosition(0, x, 0)
    x = x + 0.1

end
