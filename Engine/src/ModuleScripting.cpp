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

GameObject* own = NULL;

ModuleScripting::ModuleScripting()
{
}

ModuleScripting::~ModuleScripting()
{
}
int  Lua_SetPosition(lua_State* L)
{
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float z = luaL_checknumber(L, 3);

    GameObject* go = own;
    Transform* transform = static_cast<Transform*>(go->GetComponent(ComponentType::TRANSFORM));
    transform->SetPosition({ x, y, z });

    return 0;
}
bool ModuleScripting::Start()
{
    LOG_DEBUG("Initializing ModuleScripting");
    own = owner;
    L = luaL_newstate();
    luaL_openlibs(L);
    lua_register(L, "SetPosition", Lua_SetPosition);

    LOG_CONSOLE("ModuleScripting ready");

    return true;
}

bool ModuleScripting::Update()
{
    if (init) {
        
        //LoadScript("../Library/Scripts/test.lua");
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
    filePath = path;
    name = filePath.substr( filePath.find_last_of("/\\")+1);
    return true;
}

bool ModuleScripting::CleanUp()
{
    lua_close(L);
    LOG_DEBUG("Cleaning up ModuleScripting");
    return true;
}