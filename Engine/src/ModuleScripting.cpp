#include "ModuleScripting.h"
#include "Renderer.h"
#include "FileSystem.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"
#include "Transform.h"
#include <float.h>
#include <functional>
#include "ComponentMesh.h"
#include "ComponentCamera.h"
#include <nlohmann/json.hpp>
#include <fstream>

ModuleScripting::ModuleScripting() : Module()
{
    name = "ModuleScripting";
}

ModuleScripting::~ModuleScripting()
{
}

bool ModuleScripting::Awake()
{


    return true;
}
int  Lua_SetPosition(lua_State* L)
{
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float z = luaL_checknumber(L, 3);

    GameObject* go = Application::GetInstance().script.get()->owner;
    Transform* transform = static_cast<Transform*>(go->GetComponent(ComponentType::TRANSFORM));
    transform->SetPosition({ x, y, z });

    return 0;
}
bool ModuleScripting::Start()
{
    LOG_DEBUG("Initializing ModuleScripting");
    owner = Application::GetInstance().scene.get()->GetRoot();

    L = luaL_newstate();
    luaL_openlibs(L);
    lua_register(L, "SetPosition", Lua_SetPosition);

    LOG_CONSOLE("ModuleScripting ready");

    return true;
}

bool ModuleScripting::Update()
{
    if (init) {
        
        LoadScript("../Library/Scripts/test.lua");
        lua_getglobal(L, "Start");
        if (lua_isfunction(L, -1))
            lua_pcall(L, 0, 0, 0);

        init = false;
    }

    lua_getglobal(L, "Update");
    if (lua_isfunction(L, -1))
        lua_pcall(L, 0, 0, 0);

    return true;
}
bool ModuleScripting::LoadScript(const char* path)
{
    if (luaL_loadfile(L, path) != LUA_OK)
    {
        LOG_CONSOLE("Lua load error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        LOG_CONSOLE("Lua runtime error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool ModuleScripting::PostUpdate()
{
    return true;
}

bool ModuleScripting::CleanUp()
{
    lua_close(L);
    LOG_DEBUG("Cleaning up ModuleScripting");
    return true;
}