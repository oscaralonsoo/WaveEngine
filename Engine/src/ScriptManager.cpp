#include "ScriptManager.h"
#include "Application.h"
#include "Input.h"
#include "Time.h"
#include "Log.h"
#include "Transform.h"
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
        LOG_CONSOLE("[ScriptManager] Current working directory: %s",
            std::filesystem::current_path().string().c_str());
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

// FUNCIONES EXPUESTAS A LUA

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

    lua_pushboolean(L, pressed);
    return 1;
}

static int Lua_Input_GetKeyDown(lua_State* L) {
    const char* keyName = luaL_checkstring(L, 1);

    bool pressed = false;

    if (strcmp(keyName, "Space") == 0) {
        pressed = Input::IsKeyDown(SDL_SCANCODE_SPACE);
    }

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

static int Lua_Transform_GetPosition(lua_State* L) {
    Transform* transform = *(Transform**)lua_touserdata(L, 1);
    if (!transform) {
        lua_pushnil(L);
        return 1;
    }

    const glm::vec3& pos = transform->GetPosition();

    lua_newtable(L);
    lua_pushnumber(L, pos.x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, pos.y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, pos.z);
    lua_setfield(L, -2, "z");

    return 1;
}

static int Lua_Transform_SetPosition(lua_State* L) {
    Transform* transform = *(Transform**)lua_touserdata(L, 1);
    if (!transform) return 0;

    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);

    transform->SetPosition(glm::vec3(x, y, z));
    return 0;
}

static int Lua_Transform_GetRotation(lua_State* L) {
    Transform* transform = *(Transform**)lua_touserdata(L, 1);
    if (!transform) {
        lua_pushnil(L);
        return 1;
    }

    const glm::vec3& rot = transform->GetRotation();

    lua_newtable(L);
    lua_pushnumber(L, rot.x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, rot.y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, rot.z);
    lua_setfield(L, -2, "z");

    return 1;
}

static int Lua_Transform_SetRotation(lua_State* L) {
    Transform* transform = *(Transform**)lua_touserdata(L, 1);
    if (!transform) return 0;

    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float z = (float)luaL_checknumber(L, 4);

    transform->SetRotation(glm::vec3(x, y, z));
    return 0;
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