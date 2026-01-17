#pragma once

#include "Component.h"
#include "ModuleResources.h"
#include <string>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

class Transform;  

class ComponentScript : public Component {
public:
    ComponentScript(GameObject* owner);
    ~ComponentScript() override;

    // Component lifecycle
    void Enable() override;
    void Update() override;
    void Disable() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Script management
    bool LoadScriptByUID(UID scriptUID);
    void UnloadScript();
    bool ReloadScript();

    // Lua lifecycle
    void CallStart();
    void CallUpdate(float deltaTime);

    // Getters
    UID GetScriptUID() const { return scriptUID; }
    bool HasScript() const { return scriptUID != 0; }
    const std::string& GetLuaTableName() const { return luaTableName; }

private:
    void CreateLuaTable();
    void DestroyLuaTable();
    void SetupLuaEnvironment();
    bool CompileAndExecuteScript(const std::string& scriptContent);

    void SetupScriptEnvironment(lua_State* L);
    void CreateTransformUserdata(lua_State* L, Transform* transform);

    UID scriptUID = 0;
    std::string luaTableName;  
    bool startCalled = false;
};