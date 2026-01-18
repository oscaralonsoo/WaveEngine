#include "ScriptManager.h"
#include "Application.h"
#include "Input.h"
#include "Time.h"
#include "Log.h"
#include "Transform.h"
#include "GameObject.h"
#include "ModuleScene.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ComponentScript.h"
#include "ModuleResources.h"
#include "PrefabManager.h"  
#include <SDL3/SDL_scancode.h>
#include <filesystem>

ScriptManager::ScriptManager() : Module(), L(nullptr) {
    name = "ScriptManager";
}

ScriptManager::~ScriptManager() {
}

bool ScriptManager::Awake() {
    return true;
}

bool ScriptManager::Start() {
    LOG_CONSOLE("[ScriptManager] Initializing Lua...");

    L = luaL_newstate();
    if (!L) {
        LOG_CONSOLE("[ScriptManager] ERROR: Failed to create Lua state");
        return false;
    }

    luaL_openlibs(L);
    LOG_CONSOLE("[ScriptManager] Lua initialized");

    RegisterEngineFunctions();
    RegisterGameObjectAPI();
    RegisterComponentAPI();
    RegisterPrefabAPI();  

    LOG_CONSOLE("[ScriptManager] Started successfully");
    return true;
}

bool ScriptManager::Update() {
    if (!L) return true;
    return true;
}

bool ScriptManager::CleanUp() {
    if (L) {
        lua_close(L);
        L = nullptr;
    }

    loadedScripts.clear();
    LOG_CONSOLE("[ScriptManager] Cleaned up");
    return true;
}

bool ScriptManager::LoadScript(const std::string& filepath) {
    if (!std::filesystem::exists(filepath)) {
        LOG_CONSOLE("[ScriptManager] ERROR: Script not found: %s", filepath.c_str());
        return false;
    }

    LOG_CONSOLE("[ScriptManager] Loading script: %s", filepath.c_str());

    int result = luaL_dofile(L, filepath.c_str());

    if (result != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        LOG_CONSOLE("[ScriptManager] ERROR: %s", error);
        lua_pop(L, 1);
        return false;
    }

    loadedScripts[filepath] = true;
    LOG_CONSOLE("[ScriptManager] Script loaded: %s", filepath.c_str());
    return true;
}

bool ScriptManager::ReloadScript(const std::string& filepath) {
    LOG_CONSOLE("[ScriptManager] Reloading: %s", filepath.c_str());
    return LoadScript(filepath);
}

void ScriptManager::CallGlobalStart() {
    lua_getglobal(L, "Start");

    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            LOG_CONSOLE("[ScriptManager] ERROR in Start(): %s", error);
            lua_pop(L, 1);
        }
    }
    else {
        lua_pop(L, 1);
    }
}

void ScriptManager::CallGlobalUpdate(float deltaTime) {
    lua_getglobal(L, "Update");

    if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, deltaTime);

        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            LOG_CONSOLE("[ScriptManager] ERROR in Update(): %s", error);
            lua_pop(L, 1);
        }
    }
    else {
        lua_pop(L, 1);
    }
}

bool ScriptManager::HasGlobalFunction(const std::string& functionName) {
    lua_getglobal(L, functionName.c_str());
    bool isFunc = lua_isfunction(L, -1);
    lua_pop(L, 1);
    return isFunc;
}


//basic lua functions

static int Lua_Engine_Log(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    LOG_CONSOLE("[Lua] %s", message);
    return 0;
}

static int Lua_Input_GetKey(lua_State* L) {
    const char* keyName = luaL_checkstring(L, 1);

    bool pressed = false;

    if (strcmp(keyName, "W") == 0) pressed = Input::IsKeyPressed(SDL_SCANCODE_W);
    else if (strcmp(keyName, "A") == 0) pressed = Input::IsKeyPressed(SDL_SCANCODE_A);
    else if (strcmp(keyName, "S") == 0) pressed = Input::IsKeyPressed(SDL_SCANCODE_S);
    else if (strcmp(keyName, "D") == 0) pressed = Input::IsKeyPressed(SDL_SCANCODE_D);
    else if (strcmp(keyName, "Space") == 0) pressed = Input::IsKeyPressed(SDL_SCANCODE_SPACE);
    else if (strcmp(keyName, "Q") == 0) pressed = Input::IsKeyPressed(SDL_SCANCODE_Q);
    else if (strcmp(keyName, "E") == 0) pressed = Input::IsKeyPressed(SDL_SCANCODE_E);

    lua_pushboolean(L, pressed);
    return 1;
}

static int Lua_Input_GetKeyDown(lua_State* L) {
    const char* keyName = luaL_checkstring(L, 1);

    bool pressed = false;

    if (strcmp(keyName, "Space") == 0) pressed = Input::IsKeyDown(SDL_SCANCODE_SPACE);
    else if (strcmp(keyName, "W") == 0) pressed = Input::IsKeyDown(SDL_SCANCODE_W);
    else if (strcmp(keyName, "A") == 0) pressed = Input::IsKeyDown(SDL_SCANCODE_A);
    else if (strcmp(keyName, "S") == 0) pressed = Input::IsKeyDown(SDL_SCANCODE_S);
    else if (strcmp(keyName, "D") == 0) pressed = Input::IsKeyDown(SDL_SCANCODE_D);
    else if (strcmp(keyName, "Q") == 0) pressed = Input::IsKeyDown(SDL_SCANCODE_Q);
    else if (strcmp(keyName, "E") == 0) pressed = Input::IsKeyDown(SDL_SCANCODE_E);

    lua_pushboolean(L, pressed);
    return 1;
}

static int Lua_Input_GetMousePosition(lua_State* L) {
    int x, y;
    Input::GetMousePosition(x, y);

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

static int Lua_Time_GetDeltaTime(lua_State* L) {
    lua_pushnumber(L, Time::GetDeltaTimeStatic());
    return 1;
}

void ScriptManager::RegisterEngineFunctions() {
    if (!L) {
        LOG_CONSOLE("[ScriptManager] ERROR: Cannot register functions, Lua state is null");
        return;
    }

    LOG_CONSOLE("[ScriptManager] Registering engine functions...");

    // Engine.Log
    lua_newtable(L);
    lua_pushcfunction(L, Lua_Engine_Log);
    lua_setfield(L, -2, "Log");
    lua_setglobal(L, "Engine");

    // Input
    lua_newtable(L);
    lua_pushcfunction(L, Lua_Input_GetKey);
    lua_setfield(L, -2, "GetKey");
    lua_pushcfunction(L, Lua_Input_GetKeyDown);
    lua_setfield(L, -2, "GetKeyDown");
    lua_pushcfunction(L, Lua_Input_GetMousePosition);
    lua_setfield(L, -2, "GetMousePosition");
    lua_setglobal(L, "Input");

    // Time
    lua_newtable(L);
    lua_pushcfunction(L, Lua_Time_GetDeltaTime);
    lua_setfield(L, -2, "GetDeltaTime");
    lua_setglobal(L, "Time");

    LOG_CONSOLE("[ScriptManager] Engine functions registered: Engine, Input, Time");
}


// API DE GAMEOBJECT


// GameObject.Create(name)
static int Lua_GameObject_Create(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    GameObject* obj = Application::GetInstance().scene->CreateGameObject(name);

    if (!obj) {
        lua_pushnil(L);
        return 1;
    }

    GameObject* root = Application::GetInstance().scene->GetRoot();
    if (root && obj->GetParent() == nullptr) {
        root->AddChild(obj);
        LOG_DEBUG("[Lua] GameObject added to scene root: %s", name);
    }

    // Crear userdata para el GameObject
    GameObject** udata = (GameObject**)lua_newuserdata(L, sizeof(GameObject*));
    *udata = obj;

    // Asignar metatable
    luaL_getmetatable(L, "GameObject");
    lua_setmetatable(L, -2);

    LOG_DEBUG("[Lua] Created GameObject: %s", name);

    return 1;
}

// GameObject.Destroy(gameObject)
static int Lua_GameObject_Destroy(lua_State* L) {
    GameObject* obj = *(GameObject**)luaL_checkudata(L, 1, "GameObject");

    if (obj) {
        obj->MarkForDeletion();
        LOG_DEBUG("[Lua] Marked GameObject '%s' for deletion", obj->GetName().c_str());
    }

    return 0;
}

// GameObject.Find(name)
static int Lua_GameObject_Find(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    GameObject* root = Application::GetInstance().scene->GetRoot();
    if (!root) {
        lua_pushnil(L);
        return 1;
    }

    // Buscar recursivamente
    std::function<GameObject* (GameObject*)> findRecursive = [&](GameObject* parent) -> GameObject* {
        if (parent->GetName() == name) {
            return parent;
        }

        for (GameObject* child : parent->GetChildren()) {
            GameObject* found = findRecursive(child);
            if (found) return found;
        }

        return nullptr;
        };

    GameObject* found = findRecursive(root);

    if (!found) {
        lua_pushnil(L);
        return 1;
    }

    // Devolver userdata
    GameObject** udata = (GameObject**)lua_newuserdata(L, sizeof(GameObject*));
    *udata = found;

    luaL_getmetatable(L, "GameObject");
    lua_setmetatable(L, -2);

    return 1;
}

// gameObject.__index
static int Lua_GameObject_Index(lua_State* L) {
    GameObject* obj = *(GameObject**)luaL_checkudata(L, 1, "GameObject");
    const char* key = luaL_checkstring(L, 2);

    if (strcmp(key, "name") == 0) {
        lua_pushstring(L, obj->GetName().c_str());
        return 1;
    }

    if (strcmp(key, "transform") == 0) {
        Transform* transform = static_cast<Transform*>(obj->GetComponent(ComponentType::TRANSFORM));
        if (!transform) {
            lua_pushnil(L);
            return 1;
        }

        Transform** udata = (Transform**)lua_newuserdata(L, sizeof(Transform*));
        *udata = transform;

        luaL_getmetatable(L, "Transform");
        lua_setmetatable(L, -2);

        return 1;
    }

    // Métodos
    if (strcmp(key, "AddScript") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            GameObject* obj = *(GameObject**)luaL_checkudata(L, 1, "GameObject");
            const char* scriptPath = luaL_checkstring(L, 2);

            // Crear ComponentScript
            ComponentScript* script = static_cast<ComponentScript*>(obj->CreateComponent(ComponentType::SCRIPT));

            if (!script) {
                lua_pushboolean(L, false);
                return 1;
            }

            // Buscar UID del script
            ModuleResources* resources = Application::GetInstance().resources.get();
            UID scriptUID = resources->Find(scriptPath);

            if (scriptUID == 0) {
                LOG_CONSOLE("[Lua] ERROR: Script not found: %s", scriptPath);
                lua_pushboolean(L, false);
                return 1;
            }

            // Cargar script
            bool success = script->LoadScriptByUID(scriptUID);
            lua_pushboolean(L, success);

            return 1;
            });
        return 1;
    }

    if (strcmp(key, "SetMesh") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            GameObject* obj = *(GameObject**)luaL_checkudata(L, 1, "GameObject");
            const char* meshPath = luaL_checkstring(L, 2);

            ComponentMesh* mesh = static_cast<ComponentMesh*>(obj->GetComponent(ComponentType::MESH));
            if (!mesh) {
                mesh = static_cast<ComponentMesh*>(obj->CreateComponent(ComponentType::MESH));
            }

            ModuleResources* resources = Application::GetInstance().resources.get();
            UID meshUID = resources->Find(meshPath);

            if (meshUID == 0) {
                LOG_CONSOLE("[Lua] ERROR: Mesh not found: %s", meshPath);
                lua_pushboolean(L, false);
                return 1;
            }

            bool success = mesh->LoadMeshByUID(meshUID);
            lua_pushboolean(L, success);

            return 1;
            });
        return 1;
    }

    if (strcmp(key, "SetMaterial") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            GameObject* obj = *(GameObject**)luaL_checkudata(L, 1, "GameObject");
            const char* texturePath = luaL_checkstring(L, 2);

            ComponentMaterial* mat = static_cast<ComponentMaterial*>(obj->GetComponent(ComponentType::MATERIAL));
            if (!mat) {
                mat = static_cast<ComponentMaterial*>(obj->CreateComponent(ComponentType::MATERIAL));
            }

            ModuleResources* resources = Application::GetInstance().resources.get();
            UID textureUID = resources->Find(texturePath);

            if (textureUID == 0) {
                LOG_CONSOLE("[Lua] ERROR: Texture not found: %s", texturePath);
                lua_pushboolean(L, false);
                return 1;
            }

            bool success = mat->LoadTextureByUID(textureUID);
            lua_pushboolean(L, success);

            return 1;
            });
        return 1;
    }

    return 0;
}

void ScriptManager::RegisterGameObjectAPI() {
    // Crear metatable para GameObject
    luaL_newmetatable(L, "GameObject");

    lua_pushcfunction(L, Lua_GameObject_Index);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);

    // Crear tabla GameObject global
    lua_newtable(L);

    lua_pushcfunction(L, Lua_GameObject_Create);
    lua_setfield(L, -2, "Create");

    lua_pushcfunction(L, Lua_GameObject_Destroy);
    lua_setfield(L, -2, "Destroy");

    lua_pushcfunction(L, Lua_GameObject_Find);
    lua_setfield(L, -2, "Find");

    lua_setglobal(L, "GameObject");

    LOG_CONSOLE("[ScriptManager] GameObject API registered");
}

// API DE TRANSFORM

static int Lua_Transform_Index(lua_State* L) {
    Transform* t = *(Transform**)luaL_checkudata(L, 1, "Transform");
    const char* key = luaL_checkstring(L, 2);

    if (strcmp(key, "position") == 0) {
        const glm::vec3& pos = t->GetPosition();

        lua_newtable(L);
        lua_pushnumber(L, pos.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, pos.y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, pos.z);
        lua_setfield(L, -2, "z");

        return 1;
    }

    if (strcmp(key, "rotation") == 0) {
        const glm::vec3& rot = t->GetRotation();

        lua_newtable(L);
        lua_pushnumber(L, rot.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, rot.y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, rot.z);
        lua_setfield(L, -2, "z");

        return 1;
    }

    if (strcmp(key, "scale") == 0) {
        const glm::vec3& scl = t->GetScale();

        lua_newtable(L);
        lua_pushnumber(L, scl.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, scl.y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, scl.z);
        lua_setfield(L, -2, "z");

        return 1;
    }

    if (strcmp(key, "SetPosition") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            Transform* t = *(Transform**)luaL_checkudata(L, 1, "Transform");
            float x = (float)luaL_checknumber(L, 2);
            float y = (float)luaL_checknumber(L, 3);
            float z = (float)luaL_checknumber(L, 4);
            t->SetPosition(glm::vec3(x, y, z));
            return 0;
            });
        return 1;
    }

    if (strcmp(key, "SetRotation") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            Transform* t = *(Transform**)luaL_checkudata(L, 1, "Transform");
            float x = (float)luaL_checknumber(L, 2);
            float y = (float)luaL_checknumber(L, 3);
            float z = (float)luaL_checknumber(L, 4);
            t->SetRotation(glm::vec3(x, y, z));
            return 0;
            });
        return 1;
    }

    if (strcmp(key, "SetScale") == 0) {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            Transform* t = *(Transform**)luaL_checkudata(L, 1, "Transform");
            float x = (float)luaL_checknumber(L, 2);
            float y = (float)luaL_checknumber(L, 3);
            float z = (float)luaL_checknumber(L, 4);
            t->SetScale(glm::vec3(x, y, z));
            return 0;
            });
        return 1;
    }

    return 0;
}

void ScriptManager::RegisterComponentAPI() {
    // Transform metatable
    luaL_newmetatable(L, "Transform");

    lua_pushcfunction(L, Lua_Transform_Index);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);

    LOG_CONSOLE("[ScriptManager] Transform API registered");
}

//NUEVA API DE PREFABS

// Prefab.Load(name, filepath)
static int Lua_Prefab_Load(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    const char* filepath = luaL_checkstring(L, 2);

    bool success = PrefabManager::GetInstance().LoadPrefab(name, filepath);

    lua_pushboolean(L, success);
    return 1;
}

// Prefab.Instantiate(name)
static int Lua_Prefab_Instantiate(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    GameObject* instance = PrefabManager::GetInstance().InstantiatePrefab(name);

    if (!instance) {
        lua_pushnil(L);
        return 1;
    }

    // Devolver userdata
    GameObject** udata = (GameObject**)lua_newuserdata(L, sizeof(GameObject*));
    *udata = instance;

    luaL_getmetatable(L, "GameObject");
    lua_setmetatable(L, -2);

    return 1;
}

void ScriptManager::RegisterPrefabAPI() {
    lua_newtable(L);

    lua_pushcfunction(L, Lua_Prefab_Load);
    lua_setfield(L, -2, "Load");

    lua_pushcfunction(L, Lua_Prefab_Instantiate);
    lua_setfield(L, -2, "Instantiate");

    lua_setglobal(L, "Prefab");

    LOG_CONSOLE("[ScriptManager] Prefab API registered");
}