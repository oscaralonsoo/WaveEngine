#pragma once
#include "Module.h"
#include <memory>
#include <vector>
#include <lua.hpp>

class GameObject;
class FileSystem;
class Renderer;
class ComponentCamera;
class SceneWindow;

class ModuleScripting : public Module
{
public:
    ModuleScripting();
    virtual ~ModuleScripting();

    bool Awake() override;
    bool Start() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;

    bool LoadScript(const char* path);
    //int Lua_SetPosition(lua_State* L);

    GameObject* owner = NULL;

private:
    lua_State* L;
    bool init = true;
};
