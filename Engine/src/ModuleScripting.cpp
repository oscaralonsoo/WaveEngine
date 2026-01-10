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
#include "ComponentMaterial.h"
#include "ComponentCamera.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include "Log.h"

ModuleScripting::ModuleScripting()
{
}

ModuleScripting::~ModuleScripting()
{
}
int  Lua_SetPosition(lua_State* L)
{
    GameObject* go = (GameObject*)lua_touserdata(L, 1);
    if (go == NULL)return 0;

    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float z = luaL_checknumber(L, 4);

    Transform* transform = static_cast<Transform*>(go->GetComponent(ComponentType::TRANSFORM));
    transform->SetPosition({ x, y, z });

    return 1;
}
int  Lua_SetRotation(lua_State* L)
{
    GameObject* go = (GameObject*)lua_touserdata(L, 1);
    if (go == NULL)return 0;
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float z = luaL_checknumber(L, 4);

    Transform* transform = static_cast<Transform*>(go->GetComponent(ComponentType::TRANSFORM));
    transform->SetRotation({ x, y, z });

    return 1;
}
int  Lua_SetScale(lua_State* L)
{
    GameObject* go = (GameObject*)lua_touserdata(L, 1);
    if (go == NULL)return 0;
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float z = luaL_checknumber(L, 4);

    Transform* transform = static_cast<Transform*>(go->GetComponent(ComponentType::TRANSFORM));
    transform->SetScale({ x, y, z });

    return 1;
}
int Lua_GetPosition(lua_State* L)
{
    GameObject* go = (GameObject*)lua_touserdata(L, 1);
    if (go == NULL)return 0;

    Transform* transform = static_cast<Transform*>(go->GetComponent(ComponentType::TRANSFORM));
    glm::vec3 pos = transform->GetPosition();
    lua_newtable(L);
    
    lua_pushnumber(L,pos.x);
    lua_setfield(L, -2, "x");

    lua_pushnumber(L, pos.y);
    lua_setfield(L, -2, "y");

    lua_pushnumber(L, pos.z);
    lua_setfield(L, -2, "z");

    return 1;
}
int Lua_GetRotation(lua_State* L)
{
    GameObject* go = (GameObject*)lua_touserdata(L, 1);
    if (go == NULL)return 0;

    Transform* transform = static_cast<Transform*>(go->GetComponent(ComponentType::TRANSFORM));
    glm::vec3 pos = transform->GetRotation();
    lua_newtable(L);

    lua_pushnumber(L, pos.x);
    lua_setfield(L, -2, "x");

    lua_pushnumber(L, pos.y);
    lua_setfield(L, -2, "y");

    lua_pushnumber(L, pos.z);
    lua_setfield(L, -2, "z");

    return 1;
}
void GetAllGameObjects(GameObject* root, std::vector<GameObject*>& outObjects)
{
    if (root == nullptr)
        return;

    outObjects.push_back(root);

    const std::vector<GameObject*>& children = root->GetChildren();
    for (GameObject* child : children)
    {
        GetAllGameObjects(child, outObjects);
    }
}
int Lua_FindGameObject(lua_State* L)
{
    ModuleScripting* self = (ModuleScripting*)lua_touserdata(L, lua_upvalueindex(1));

    std::string ObjName = luaL_checkstring(L, 1);
    GameObject* go = NULL;

    if (ObjName != "this")
    {
        std::vector<GameObject*> objects;
        GetAllGameObjects(Application::GetInstance().scene.get()->GetRoot(), objects);

        for (auto& obj : objects)
            if (obj->GetName() == ObjName) go = obj;

        if (!go)
        {
            //LOG_CONSOLE("[Script] %s error: GameObject %s not found",self->name.c_str(), ObjName.c_str());
            LOG_CONSOLE("ERROR [Script] %s: GameObject %s not found", self->name.c_str(), ObjName.c_str());
            lua_pushnil(L);
            return 0;
        }
    }
    else go = self->owner;

    lua_pushlightuserdata(L, go);
    return 1;
}
int Lua_CreatePrimitiveGameObject(lua_State* L)
{
    ModuleScripting* self = (ModuleScripting*)lua_touserdata(L, lua_upvalueindex(1));

    std::string name = luaL_checkstring(L, 1);
    std::string Objname = luaL_checkstring(L, 2);

    GameObject* Object = new GameObject(name);
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(
        Object->CreateComponent(ComponentType::MESH)
        );

    Mesh selectedMesh;

    if (name == "Cube")
        selectedMesh = Primitives::CreateCube();
    else if (name == "Pyramid")
        selectedMesh = Primitives::CreatePyramid();
    else if (name == "Plane")
        selectedMesh = Primitives::CreatePlane();
    else if (name == "Sphere")
        selectedMesh = Primitives::CreateSphere();
    else if (name == "Cylinder")
        selectedMesh = Primitives::CreateCylinder();
    else return 0;

    meshComp->SetMesh(selectedMesh);
    meshComp->SetPrimitiveType(name);

    ComponentMaterial* materialComp = static_cast<ComponentMaterial*>(
        Object->CreateComponent(ComponentType::MATERIAL)
        );
    materialComp->CreateCheckerboardTexture();
    Object->name = Objname;

    Application::GetInstance().scene.get()->newObject.push_back(Object);

    lua_pushlightuserdata(L, Object);
    return 1;
}
bool ModuleScripting::Start()
{
    LOG_DEBUG("Initializing ModuleScripting");
    L = luaL_newstate();
    luaL_openlibs(L);

    // SetPosition
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, Lua_SetPosition, 1);
    lua_setglobal(L, "SetPosition");

    // SetRotation
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, Lua_SetRotation, 1);
    lua_setglobal(L, "SetRotation");

    // GetPosition
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, Lua_GetPosition, 1);
    lua_setglobal(L, "GetPosition");

    // GetRotation
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, Lua_GetPosition, 1);
    lua_setglobal(L, "GetPosition");

    // SetScale
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, Lua_GetRotation, 1);
    lua_setglobal(L, "GetRotation");

    // FindGameObject
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, Lua_FindGameObject, 1);
    lua_setglobal(L, "FindGameObject");

    // CreatePrimitive
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, Lua_CreatePrimitiveGameObject, 1);
    lua_setglobal(L, "CreatePrimitive");

    return true;

    LOG_CONSOLE("ModuleScripting ready");

    return true;
}

bool ModuleScripting::Update()
{
    if (scriptError) return true;

    if (init) {
        lua_getglobal(L, "Start");
        if (lua_isfunction(L, -1))
            lua_pcall(L, 0, 0, 0);

        init = false;
    }
    PushInput();

    lua_getglobal(L, "Update");
    if (lua_isfunction(L, -1))
        lua_pcall(L, 0, 0, 0);

    return true;
}
void ModuleScripting::PushInput()
{
    lua_newtable(L);
    
    bool tr = false;

    lua_pushboolean(L, Application::GetInstance().input.get()->GetKey(SDL_SCANCODE_W) == KEY_REPEAT);
    lua_setfield(L, -2, "W");

    lua_pushboolean(L, Application::GetInstance().input.get()->GetKey(SDL_SCANCODE_A) == KEY_REPEAT);
    lua_setfield(L, -2, "A");

    lua_pushboolean(L, Application::GetInstance().input.get()->GetKey(SDL_SCANCODE_S) == KEY_REPEAT);
    lua_setfield(L, -2, "S");

    lua_pushboolean(L, Application::GetInstance().input.get()->GetKey(SDL_SCANCODE_D) == KEY_REPEAT);
    lua_setfield(L, -2, "D");

    lua_pushboolean(L, Application::GetInstance().input.get()->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN);
    lua_setfield(L, -2, "MouseRight");
    
    lua_pushboolean(L, Application::GetInstance().input.get()->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN);
    lua_setfield(L, -2, "MouseLeft");
    int with, heigh;

    Application::GetInstance().window.get()->GetWindowSize(with, heigh);
    
    glm::vec3 rayDir = MouseWorldPos();

    lua_pushnumber(L, rayDir.x * 10 );
    lua_setfield(L, -2, "MouseX");
            
    lua_pushnumber(L,rayDir.y * 10);
    lua_setfield(L, -2, "MouseY");

    lua_setglobal(L, "Input");
}
glm::vec3 ModuleScripting::MouseWorldPos()
{
    float mouseXf, mouseYf;
    ComponentCamera* camera = Application::GetInstance().camera.get()->GetActiveCamera();

    SDL_GetMouseState(&mouseXf, &mouseYf);
    int scale = Application::GetInstance().window.get()->GetScale();
    mouseXf /= scale;
    mouseYf /= scale;

    ImVec2 scenePos = Application::GetInstance().editor->sceneViewportPos;
    ImVec2 sceneSize = Application::GetInstance().editor->sceneViewportSize;

    float relativeX = mouseXf - scenePos.x;
    float relativeY = mouseYf - scenePos.y;

    glm::vec3 rayOrigin = camera->GetPosition();
    return camera->ScreenToWorldRay(
        static_cast<int>(relativeX),
        static_cast<int>(relativeY),
        static_cast<int>(sceneSize.x),
        static_cast<int>(sceneSize.y)
    );
}
bool ModuleScripting::LoadScript(const char* path)
{
    scriptError = false;
    lastError.clear();

    if (luaL_loadfile(L, path) != LUA_OK)
    {
        lastError = lua_tostring(L, -1);
        LOG_CONSOLE("ERROR [Lua Load] %s", lastError.c_str());
        //LOG_CONSOLE("Lua load error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        //LOG_CONSOLE("Lua runtime error: %s", lua_tostring(L, -1));
        LOG_CONSOLE("ERROR [Lua Runtime] %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    filePath = path;
    name = filePath.substr( filePath.find_last_of("/\\")+1);

    return true;
}

bool ModuleScripting::ReloadScript(const std::string& path)
{
    lastError.clear();
    scriptError = false;

    if (luaL_loadfile(L, path.c_str()) != LUA_OK)
    {
        lastError = lua_tostring(L, -1);
        LOG_CONSOLE("ERROR [Lua Reload] %s", lastError.c_str());
        //LOG_CONSOLE("[Lua Error] %s", lastError.c_str());
        scriptError = true;
        lua_pop(L, 1);
        return false;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        lastError = lua_tostring(L, -1);
        LOG_CONSOLE("ERROR [Lua Runtime] %s", lastError.c_str());
       //LOG_CONSOLE("[Lua Runtime Error] %s", lastError.c_str());
        lastError = lua_tostring(L, -1);
        LOG_CONSOLE("ERROR %s", lastError.c_str());
        scriptError = true;
        lua_pop(L, 1);
        return false;
    }

    init = true;

    LOG_CONSOLE("[Lua] Hot-Reloaded successfully: %s", path.c_str());
    return true;
}


bool ModuleScripting::CleanUp()
{
    lua_close(L);
    LOG_DEBUG("Cleaning up ModuleScripting");
    return true;
}