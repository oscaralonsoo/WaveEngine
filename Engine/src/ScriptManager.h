#pragma once

#include "Module.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <string>
#include <unordered_map>

class GameObject;
class Transform;

class ScriptManager : public Module {
private:
    lua_State* L;

    // Cache de scripts cargados (path -> true si esta cargado)
    std::unordered_map<std::string, bool> loadedScripts;

public:
    ScriptManager();
    ~ScriptManager();

    bool Awake() override;
    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    // Cargar script desde archivo
    bool LoadScript(const std::string& filepath);

    // Recargar script (hot reload)
    bool ReloadScript(const std::string& filepath);

    // Llamar funciones globales del script
    void CallGlobalStart();
    void CallGlobalUpdate(float deltaTime);

    // Obtener estado de Lua
    lua_State* GetState() { return L; }

    // Registrar funciones C++ para Lua
    void RegisterEngineFunctions();

    // Helper: verificar si existe una función
    bool HasGlobalFunction(const std::string& functionName);

};