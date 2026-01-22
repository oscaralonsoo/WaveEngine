#include "ComponentScript.h"
#include "Application.h"
#include "ScriptManager.h"
#include "ResourceScript.h"
#include "GameObject.h"
#include "Transform.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include <filesystem>
#include "Log.h"
#include <sstream>

ComponentScript::ComponentScript(GameObject* owner)
    : Component(owner, ComponentType::SCRIPT)
{
    name = "Script";
}

ComponentScript::~ComponentScript()
{
    UnloadScript();
}

void ComponentScript::Enable()
{
    if (!HasScript()) return;

    // Llamar Start si no se ha llamado aún
    if (!startCalled && active) {
        CallStart();
    }
}

void ComponentScript::Update()
{

    if (!active) {
        return;
    }

    if (!HasScript()) {
        return;
    }

    // Solo actualizar en modo PLAYING
    auto& app = Application::GetInstance();
    if (app.GetPlayState() != Application::PlayState::PLAYING) {
        return;
    }


    // Verificar si el script necesita recargarse (hot reload)
    ModuleResources* resources = app.resources.get();
    const Resource* res = resources->GetResourceDirect(scriptUID);

    if (res && res->GetType() == Resource::SCRIPT) {
        const ResourceScript* scriptRes = static_cast<const ResourceScript*>(res);
        if (scriptRes->NeedsReload()) {
            ReloadScript();
        }
    }

    // Llamar Update del script
    float deltaTime = app.time->GetDeltaTime();


    CallUpdate(deltaTime);

    // Ejecutar destrucción diferida DESPUÉS de Update
    if (pendingDestroy) {
        owner->MarkForDeletion();
        pendingDestroy = false;
    }

}


void ComponentScript::Disable()
{
    // Podríamos llamar a OnDisable() del script aquí si lo implementamos
}

void ComponentScript::OnEditor()
{
    if (!ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Indent();

    // Mostrar información del script actual
    if (HasScript()) {
        ModuleResources* resources = Application::GetInstance().resources.get();
        const Resource* res = resources->GetResourceDirect(scriptUID);

        if (res) {
            std::string filename = std::filesystem::path(res->GetAssetFile()).filename().string();

            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Current Script:");
            ImGui::SameLine();
            ImGui::Text("%s", filename.c_str());

            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "UID: %llu", scriptUID);

            // Botón para cambiar script
            if (ImGui::Button("Change Script")) {
                ImGui::OpenPopup("SelectScript");
            }

            ImGui::SameLine();

            // Botón para eliminar script
            if (ImGui::Button("Remove Script")) {
                UnloadScript();
            }

            ImGui::SameLine();

            // Botón para recargar manualmente
            if (ImGui::Button("Reload")) {
                ReloadScript();
            }
        }
    }
    else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No script assigned");

        if (ImGui::Button("Assign Script", ImVec2(150, 0))) {
            ImGui::OpenPopup("SelectScript");
        }
    }

    // Popup para seleccionar script
    if (ImGui::BeginPopup("SelectScript")) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Select Script");
        ImGui::Separator();

        ModuleResources* resources = Application::GetInstance().resources.get();
        const auto& allResources = resources->GetAllResources();

        bool foundScripts = false;

        for (const auto& [uid, resource] : allResources) {
            if (resource->GetType() == Resource::SCRIPT) {
                foundScripts = true;

                std::string filename = std::filesystem::path(resource->GetAssetFile()).filename().string();

                bool isSelected = (scriptUID == uid);
                if (ImGui::Selectable(filename.c_str(), isSelected)) {
                    LoadScriptByUID(uid);
                    ImGui::CloseCurrentPopup();
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }

        if (!foundScripts) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No .lua scripts found in Assets/");
            ImGui::TextWrapped("Create a .lua file in Assets/Scripts/");
        }

        ImGui::EndPopup();
    }

    // Información adicional en modo DEBUG
    if (HasScript()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::TreeNode("Debug Info")) {
            ImGui::Text("Lua Table: %s", luaTableName.c_str());
            ImGui::Text("Start Called: %s", startCalled ? "Yes" : "No");
            ImGui::Text("Active: %s", active ? "Yes" : "No");

            ImGui::TreePop();
        }
    }

    ImGui::Unindent();
}

void ComponentScript::Serialize(nlohmann::json& componentObj) const
{
    componentObj["scriptUID"] = scriptUID;

    if (!publicVariables.empty()) {
        nlohmann::json varsArray = nlohmann::json::array();

        for (const auto& var : publicVariables) {
            nlohmann::json varObj;
            varObj["name"] = var.name;
            varObj["type"] = static_cast<int>(var.type);

            switch (var.type) {
            case ScriptVarType::NUMBER:
                varObj["value"] = std::get<float>(var.value);
                break;

            case ScriptVarType::STRING:
                varObj["value"] = std::get<std::string>(var.value);
                break;

            case ScriptVarType::BOOLEAN:
                varObj["value"] = std::get<bool>(var.value);
                break;

            case ScriptVarType::VEC3: {
                const glm::vec3& vec = std::get<glm::vec3>(var.value);
                varObj["value"] = { vec.x, vec.y, vec.z };
                break;
            }
            }

            varsArray.push_back(varObj);
        }

        componentObj["publicVariables"] = varsArray;
    }
}

void ComponentScript::Deserialize(const nlohmann::json& componentObj)
{
    if (componentObj.contains("scriptUID")) {
        UID uid = componentObj["scriptUID"];

        // Cargar el script primero (esto extrae variables del .lua)
        if (LoadScriptByUID(uid)) {

            if (componentObj.contains("publicVariables")) {
                const auto& varsArray = componentObj["publicVariables"];

                // Restaurar valores guardados sobre los extraídos del .lua
                for (const auto& varObj : varsArray) {
                    std::string varName = varObj["name"];
                    ScriptVarType varType = static_cast<ScriptVarType>(varObj["type"]);

                    // Buscar la variable en publicVariables por nombre
                    for (auto& var : publicVariables) {
                        if (var.name == varName && var.type == varType) {

                            // Restaurar el valor guardado
                            switch (varType) {
                            case ScriptVarType::NUMBER:
                                var.value = varObj["value"].get<float>();
                                break;

                            case ScriptVarType::STRING:
                                var.value = varObj["value"].get<std::string>();
                                break;

                            case ScriptVarType::BOOLEAN:
                                var.value = varObj["value"].get<bool>();
                                break;

                            case ScriptVarType::VEC3: {
                                auto vec = varObj["value"];
                                var.value = glm::vec3(
                                    vec[0].get<float>(),
                                    vec[1].get<float>(),
                                    vec[2].get<float>()
                                );
                                break;
                            }
                            }

                            break;
                        }
                    }
                }

                // Sincronizar a Lua con los valores restaurados
                SyncPublicVariablesToLua();

                LOG_DEBUG("[ComponentScript] Restored %zu public variables from save",
                    varsArray.size());
            }
        }
    }
}

bool ComponentScript::LoadScriptByUID(UID uid)
{
    if (uid == 0) {
        LOG_CONSOLE("[ComponentScript] ERROR: Invalid UID");
        return false;
    }

    // Descargar script anterior si existe
    if (HasScript()) {
        UnloadScript();
    }

    ModuleResources* resources = Application::GetInstance().resources.get();
    Resource* res = resources->RequestResource(uid);

    if (!res || res->GetType() != Resource::SCRIPT) {
        LOG_CONSOLE("[ComponentScript] ERROR: Resource %llu is not a script", uid);
        return false;
    }

    const ResourceScript* scriptRes = static_cast<const ResourceScript*>(res);
    const std::string& scriptContent = scriptRes->GetScriptContent();

    if (scriptContent.empty()) {
        LOG_CONSOLE("[ComponentScript] ERROR: Script content is empty");
        resources->ReleaseResource(uid);
        return false;
    }

    // Guardar UID
    scriptUID = uid;

    // Crear tabla Lua única para este componente
    CreateLuaTable();

    // Compilar y ejecutar el script
    if (!CompileAndExecuteScript(scriptContent)) {
        LOG_CONSOLE("[ComponentScript] ERROR: Failed to compile script");
        UnloadScript();
        return false;
    }

    return true;
}

void ComponentScript::UnloadScript()
{
    if (!HasScript()) return;

    // Liberar recurso
    ModuleResources* resources = Application::GetInstance().resources.get();
    resources->ReleaseResource(scriptUID);

    // Limpiar tabla Lua
    DestroyLuaTable();

    scriptUID = 0;
    startCalled = false;
}

bool ComponentScript::ReloadScript()
{
    if (!HasScript()) return false;

    std::vector<ScriptVariable> savedVariables = publicVariables;

    UID uid = scriptUID;
    UnloadScript();

    if (!LoadScriptByUID(uid)) {
        return false;
    }

    for (const auto& savedVar : savedVariables) {
        // Buscar la variable por nombre en las nuevas
        for (auto& newVar : publicVariables) {
            if (newVar.name == savedVar.name && newVar.type == savedVar.type) {
                // Mantener el valor que tenía antes del reload
                newVar.value = savedVar.value;
                break;
            }
        }
    }

    // Sincronizar a Lua
    SyncPublicVariablesToLua();

    LOG_CONSOLE("[ComponentScript] Hot reloaded and preserved variable values");
    return true;
}

void ComponentScript::CallStart()
{
    if (!HasScript() || startCalled) return;

    // Sincronizar variables públicas antes de Start
    SyncPublicVariablesToLua();

    ScriptManager* scriptManager = Application::GetInstance().scripts.get();
    lua_State* L = scriptManager->GetState();

    if (!L) return;

    lua_getglobal(L, luaTableName.c_str());

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, "Start");

    if (lua_isfunction(L, -1)) {
        lua_pushvalue(L, -2);

        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            LOG_CONSOLE("[ComponentScript] ERROR in Start(): %s", error);
            lua_pop(L, 1);
        }
        else {
            startCalled = true;
        }
    }
    else {
        lua_pop(L, 1);
    }

    lua_pop(L, 1);
}

void ComponentScript::CallUpdate(float deltaTime)
{
    // Validaciones previas
    if (!active) {
        return;
    }

    if (!HasScript()) {
        return;
    }

    if (!owner || owner->IsMarkedForDeletion()) {
        return;
    }

    // Verificar que estamos en modo PLAYING
    auto& app = Application::GetInstance();
    if (app.GetPlayState() != Application::PlayState::PLAYING) {
        return;
    }

    ScriptManager* scriptManager = app.scripts.get();
    lua_State* L = scriptManager->GetState();

    if (!L) {
        LOG_CONSOLE("[ComponentScript] ERROR: Lua state is null");
        return;
    }

    // Hot reload check
    ModuleResources* resources = app.resources.get();
    const Resource* res = resources->GetResourceDirect(scriptUID);

    if (res && res->GetType() == Resource::SCRIPT) {
        const ResourceScript* scriptRes = static_cast<const ResourceScript*>(res);
        if (scriptRes->NeedsReload()) {
            ReloadScript();
        }
    }

    // Obtener la tabla del script
    lua_getglobal(L, luaTableName.c_str());

    if (!lua_istable(L, -1)) {
        LOG_CONSOLE("[ComponentScript] ERROR: Script table not found: %s (Owner: %s)",
            luaTableName.c_str(), owner->GetName().c_str());
        lua_pop(L, 1);
        return;
    }

    // Obtener la función Update
    lua_getfield(L, -1, "Update");

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);  // Pop function + table
        return;
    }

    // Push self (la tabla del script)
    lua_pushvalue(L, -2);

    // Push deltaTime
    lua_pushnumber(L, deltaTime);

    // Ejecutar Update(self, deltaTime)
    if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
        const char* error = lua_tostring(L, -1);

        LOG_CONSOLE("[ComponentScript] LUA ERROR in Update() - %s: %s", owner->GetName().c_str(), error ? error : "Unknown error");

        lua_pop(L, 1);
    }

    lua_pop(L, 1);  // Pop table

    // Ejecutar destrucción diferida DESPUÉS de Update
    if (pendingDestroy) {
        owner->MarkForDeletion();
        pendingDestroy = false;
    }
}

void ComponentScript::CreateLuaTable()
{
    ScriptManager* scriptManager = Application::GetInstance().scripts.get();
    lua_State* L = scriptManager->GetState();

    if (!L) return;

    // Generar nombre único para la tabla: "Script_GameObjectName_UID"
    std::stringstream ss;
    ss << "Script_" << owner->GetName() << "_" << scriptUID << "_" << (uintptr_t)this;
    luaTableName = ss.str();

    // Crear nueva tabla en Lua
    lua_newtable(L);
    lua_setglobal(L, luaTableName.c_str());
}

void ComponentScript::DestroyLuaTable()
{
    if (luaTableName.empty()) return;

    ScriptManager* scriptManager = Application::GetInstance().scripts.get();
    lua_State* L = scriptManager->GetState();

    if (!L) return;

    // Establecer la tabla como nil para liberarla
    lua_pushnil(L);
    lua_setglobal(L, luaTableName.c_str());

    luaTableName.clear();
}

void ComponentScript::SetupLuaEnvironment()
{
    // Aquí podríamos configurar el entorno Lua del script
    // Por ejemplo, establecer variables específicas del GameObject
}
bool ComponentScript::CompileAndExecuteScript(const std::string& scriptContent)
{
    ScriptManager* scriptManager = Application::GetInstance().scripts.get();
    lua_State* L = scriptManager->GetState();

    if (!L) return false;

    std::string cleanContent = scriptContent;
    if (cleanContent.size() >= 3 &&
        (unsigned char)cleanContent[0] == 0xEF &&
        (unsigned char)cleanContent[1] == 0xBB &&
        (unsigned char)cleanContent[2] == 0xBF) {
        cleanContent = cleanContent.substr(3);
    }

    int loadResult = luaL_loadstring(L, cleanContent.c_str());

    if (loadResult != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        LOG_CONSOLE("[ComponentScript] ERROR compiling script: %s", error);
        lua_pop(L, 1);
        return false;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        LOG_CONSOLE("[ComponentScript] ERROR executing script: %s", error);
        lua_pop(L, 1);
        return false;
    }

    lua_getglobal(L, luaTableName.c_str());

    if (lua_istable(L, -1)) {
        // Copiar Start si existe
        lua_getglobal(L, "Start");
        if (lua_isfunction(L, -1)) {
            lua_setfield(L, -2, "Start");
        }
        else {
            lua_pop(L, 1);
        }

        // Copiar Update si existe
        lua_getglobal(L, "Update");
        if (lua_isfunction(L, -1)) {
            lua_setfield(L, -2, "Update");
        }
        else {
            lua_pop(L, 1);
        }

        // Copiar tabla "public" global si existe
        lua_getglobal(L, "public");
        if (lua_istable(L, -1)) {
            lua_setfield(L, -2, "public");
        }
        else {
            lua_pop(L, 1);
        }

        SetupScriptEnvironment(L);
    }

    lua_pop(L, 1);

    // Extraer variables públicas
    ExtractPublicVariables();

    return true;
}

void ComponentScript::SetupScriptEnvironment(lua_State* L)
{
    // La tabla del script ya está en el stack (-1)

    // Crear userdata para el GameObject real
    GameObject** goUserdata = (GameObject**)lua_newuserdata(L, sizeof(GameObject*));
    *goUserdata = owner;

    // Asignar metatable GameObject
    luaL_getmetatable(L, "GameObject");
    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "gameObject");  // script.gameObject = userdata

    // Guardar puntero al ComponentScript para destrucción diferida
    ComponentScript** scriptUserdata = (ComponentScript**)lua_newuserdata(L, sizeof(ComponentScript*));
    *scriptUserdata = this;
    lua_setfield(L, -2, "__componentScript");

    // Función helper Destroy() que MARCA para destruir
    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_getfield(L, 1, "__componentScript");

        if (!lua_isuserdata(L, -1)) {
            LOG_CONSOLE("[Lua] ERROR: __componentScript is not valid");
            return 0;
        }

        ComponentScript* script = *(ComponentScript**)lua_touserdata(L, -1);
        script->MarkGameObjectForDestroy();

        return 0;
        });
    lua_setfield(L, -2, "Destroy");

    // Inyectar transform
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform) {
        CreateTransformUserdata(L, transform);
        lua_setfield(L, -2, "transform");
    }
}

void ComponentScript::CreateTransformUserdata(lua_State* L, Transform* transform)
{
    // Crear userdata para Transform
    Transform** udata = (Transform**)lua_newuserdata(L, sizeof(Transform*));
    *udata = transform;

    // Crear metatable para Transform
    luaL_newmetatable(L, "Transform");

    // __index metamethod
    lua_pushcfunction(L, [](lua_State* L) -> int {
        Transform* t = *(Transform**)lua_touserdata(L, 1);
        const char* key = luaL_checkstring(L, 2);

        // Propiedad "position"
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

		// World position
        if (strcmp(key, "worldPosition") == 0) {
            const glm::mat4& globalMatrix = t->GetGlobalMatrix();

            lua_newtable(L);
            lua_pushnumber(L, globalMatrix[3][0]);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, globalMatrix[3][1]);
            lua_setfield(L, -2, "y");
            lua_pushnumber(L, globalMatrix[3][2]);
            lua_setfield(L, -2, "z");

            return 1;
        }

        // Propiedad "rotation"
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

        // Propiedad "scale"
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

        // Método SetPosition
        if (strcmp(key, "SetPosition") == 0) {
            lua_pushcfunction(L, [](lua_State* L) -> int {
                Transform* t = *(Transform**)lua_touserdata(L, 1);
                float x = (float)luaL_checknumber(L, 2);
                float y = (float)luaL_checknumber(L, 3);
                float z = (float)luaL_checknumber(L, 4);
                t->SetPosition(glm::vec3(x, y, z));
                return 0;
                });
            return 1;
        }

        // Método SetRotation
        if (strcmp(key, "SetRotation") == 0) {
            lua_pushcfunction(L, [](lua_State* L) -> int {
                Transform* t = *(Transform**)lua_touserdata(L, 1);
                float x = (float)luaL_checknumber(L, 2);
                float y = (float)luaL_checknumber(L, 3);
                float z = (float)luaL_checknumber(L, 4);
                t->SetRotation(glm::vec3(x, y, z));
                return 0;
                });
            return 1;
        }

        // Método SetScale
        if (strcmp(key, "SetScale") == 0) {
            lua_pushcfunction(L, [](lua_State* L) -> int {
                Transform* t = *(Transform**)lua_touserdata(L, 1);
                float x = (float)luaL_checknumber(L, 2);
                float y = (float)luaL_checknumber(L, 3);
                float z = (float)luaL_checknumber(L, 4);
                t->SetScale(glm::vec3(x, y, z));
                return 0;
                });
            return 1;
        }

        return 0;
        });
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);
}

void ComponentScript::ExtractPublicVariables()
{
    publicVariables.clear();

    if (!HasScript()) return;

    ScriptManager* scriptManager = Application::GetInstance().scripts.get();
    lua_State* L = scriptManager->GetState();

    if (!L) return;

    // 2. Dentro de la tabla del script: script.public = {...}

    bool foundPublic = false;

    lua_getglobal(L, "public");

    if (lua_istable(L, -1)) {
        foundPublic = true;
    }
    else {
        // Si no existe global, buscar en la tabla del script
        lua_pop(L, 1);
        lua_getglobal(L, luaTableName.c_str());

        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "public");

            if (lua_istable(L, -1)) {
                foundPublic = true;
                // Intercambiar para que "public" esté en top
                lua_remove(L, -2);
            }
            else {
                lua_pop(L, 2);
            }
        }
        else {
            lua_pop(L, 1);
        }
    }

    if (!foundPublic) {
        return;
    }

    // Ahora tenemos la tabla "public" en el top del stack
    // Iterar sobre todas sus variables
    lua_pushnil(L);

    while (lua_next(L, -2) != 0) {
        if (lua_isstring(L, -2)) {
            const char* varName = lua_tostring(L, -2);

            if (lua_isnumber(L, -1)) {
                float value = (float)lua_tonumber(L, -1);
                publicVariables.emplace_back(varName, value);
            }
            else if (lua_isstring(L, -1)) {
                const char* value = lua_tostring(L, -1);
                publicVariables.emplace_back(varName, std::string(value));
            }
            else if (lua_isboolean(L, -1)) {
                bool value = lua_toboolean(L, -1);
                publicVariables.emplace_back(varName, value);
            }
            else if (lua_istable(L, -1)) {
                // Verificar si es un vec3 (tabla con x, y, z)
                lua_getfield(L, -1, "x");
                lua_getfield(L, -2, "y");
                lua_getfield(L, -3, "z");

                if (lua_isnumber(L, -3) && lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
                    float x = (float)lua_tonumber(L, -3);
                    float y = (float)lua_tonumber(L, -2);
                    float z = (float)lua_tonumber(L, -1);
                    publicVariables.emplace_back(varName, glm::vec3(x, y, z));
                }

                lua_pop(L, 3);
            }
        }

        lua_pop(L, 1);
    }

    lua_pop(L, 1);  // Pop tabla "public"
}

void ComponentScript::SyncPublicVariablesToLua()
{
    if (!HasScript() || publicVariables.empty()) return;

    ScriptManager* scriptManager = Application::GetInstance().scripts.get();
    lua_State* L = scriptManager->GetState();

    if (!L) return;

    // Sincronizar en la tabla global "public" si existe
    lua_getglobal(L, "public");
    bool hasGlobalPublic = lua_istable(L, -1);

    if (hasGlobalPublic) {
        // Actualizar variables en tabla global
        for (const auto& var : publicVariables) {
            PushVariableToLua(L, var);
            lua_setfield(L, -2, var.name.c_str());
        }
    }
    lua_pop(L, 1);

    // Sincronizar en la tabla del script
    lua_getglobal(L, luaTableName.c_str());

    if (lua_istable(L, -1)) {
        // Obtener o crear tabla "public" dentro del script
        lua_getfield(L, -1, "public");

        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, "public");
        }

        // Actualizar variables en tabla del script
        for (const auto& var : publicVariables) {
            PushVariableToLua(L, var);
            lua_setfield(L, -2, var.name.c_str());
        }

        lua_pop(L, 1);  // Pop tabla "public"
    }

    lua_pop(L, 1);  // Pop tabla del script
}

void ComponentScript::PushVariableToLua(lua_State* L, const ScriptVariable& var)
{
    switch (var.type) {
    case ScriptVarType::NUMBER:
        lua_pushnumber(L, std::get<float>(var.value));
        break;

    case ScriptVarType::STRING:
        lua_pushstring(L, std::get<std::string>(var.value).c_str());
        break;

    case ScriptVarType::BOOLEAN:
        lua_pushboolean(L, std::get<bool>(var.value));
        break;

    case ScriptVarType::VEC3: {
        const glm::vec3& vec = std::get<glm::vec3>(var.value);
        lua_newtable(L);
        lua_pushnumber(L, vec.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, vec.y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, vec.z);
        lua_setfield(L, -2, "z");
        break;
    }
    }
}

void ComponentScript::UpdatePublicVariable(size_t index, const ScriptVariable& var)
{
    if (index >= publicVariables.size()) return;

    publicVariables[index] = var;
    SyncPublicVariablesToLua();
}