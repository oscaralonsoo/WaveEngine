#include "ScriptManager.h"
#include "Application.h"
#include "Input.h"
#include "Time.h"
#include "Log.h"
#include "Transform.h"
#include "GameObject.h"
#include "Component.h"
#include "ModuleScene.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ComponentScript.h"
#include "ModuleResources.h"
#include "PrefabManager.h"
#include "ResourcePrefab.h"
#include "ComponentCamera.h" 
#include "Window.h"        
#include "ModuleCamera.h"   
#include <SDL3/SDL_scancode.h>
#include "GameWindow.h"
#include "EditorWindow.h"
#include "ModuleEditor.h"
#include "ConsoleWindow.h"

#include <filesystem>
#include <cmath>            
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

// Execute all deferred operations
bool ScriptManager::PostUpdate() {
    if (!pendingOperations.empty()) {
        // Execute all pending operations
        for (auto& operation : pendingOperations) {
            operation();
        }

        pendingOperations.clear();
    }

    return true;
}

bool ScriptManager::CleanUp() {
    if (L) {
        lua_close(L);
        L = nullptr;
    }

    loadedScripts.clear();
    pendingOperations.clear();
    LOG_CONSOLE("[ScriptManager] Cleaned up");
    return true;
}

// Method to enqueue operations
void ScriptManager::EnqueueOperation(std::function<void()> operation) {
    pendingOperations.push_back(std::move(operation));
}

bool ScriptManager::LoadScript(const std::string& filepath) {
    if (!std::filesystem::exists(filepath)) {
        LOG_CONSOLE("[ScriptManager] ERROR: Script not found: %s", filepath.c_str());

        // Activar flash de error
        Application& app = Application::GetInstance();
        if (app.editor && app.editor->GetConsoleWindow()) {
            app.editor->GetConsoleWindow()->FlashError();
        }

        return false;
    }

    int result = luaL_dofile(L, filepath.c_str());

    if (result != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        LOG_CONSOLE("[ScriptManager] ERROR: %s", error);
        lua_pop(L, 1);

        // Activar flash de error
        Application& app = Application::GetInstance();
        if (app.editor && app.editor->GetConsoleWindow()) {
            app.editor->GetConsoleWindow()->FlashError();
        }

        return false;
    }

    loadedScripts[filepath] = true;
    return true;
}
bool ScriptManager::ReloadScript(const std::string& filepath) {
    return LoadScript(filepath);
}

void ScriptManager::CallGlobalStart() {
    lua_getglobal(L, "Start");

    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            LOG_CONSOLE("[ScriptManager] ERROR in Start(): %s", error);
            lua_pop(L, 1);

            // Activar flash de error
            auto& app = Application::GetInstance();
            if (app.editor && app.editor->GetConsoleWindow()) {
                app.editor->GetConsoleWindow()->FlashError();
            }
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

            // Activar flash de error
            auto& app = Application::GetInstance();
            if (app.editor && app.editor->GetConsoleWindow()) {
                app.editor->GetConsoleWindow()->FlashError();
            }
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
    int rawX, rawY;
    Input::GetMousePosition(rawX, rawY);

    auto& app = Application::GetInstance();

    // Get the Game window directly
    GameWindow* gameWindow = app.editor->GetGameWindow();

    if (gameWindow) {
        ImVec2 viewportPos = gameWindow->GetViewportPos();
        ImVec2 viewportSize = gameWindow->GetViewportSize();

        // Convert to viewport-relative coordinates
        int relativeX = rawX - static_cast<int>(viewportPos.x);
        int relativeY = rawY - static_cast<int>(viewportPos.y);

        // Check if mouse is inside the game viewport
        if (relativeX >= 0 && relativeX < viewportSize.x &&
            relativeY >= 0 && relativeY < viewportSize.y) {
            lua_pushnumber(L, static_cast<lua_Number>(relativeX));
            lua_pushnumber(L, static_cast<lua_Number>(relativeY));
            return 2;
        }
    }

    // If not in game window, return nil
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
}
static int Lua_Time_GetDeltaTime(lua_State* L) {
    lua_pushnumber(L, Time::GetDeltaTimeStatic());
    return 1;
}

static int Lua_Camera_GetScreenToWorldPlane(lua_State* L) {
    int mouseX = static_cast<int>(luaL_checknumber(L, 1));
    int mouseY = static_cast<int>(luaL_checknumber(L, 2));
    float planeY = static_cast<float>(luaL_optnumber(L, 3, 0.0));

    auto& app = Application::GetInstance();

    // Get Game window dimensions
    int screenWidth = 800;
    int screenHeight = 600;

    GameWindow* gameWindow = app.editor->GetGameWindow();
    if (gameWindow) {
        ImVec2 viewportSize = gameWindow->GetViewportSize();
        screenWidth = static_cast<int>(viewportSize.x);
        screenHeight = static_cast<int>(viewportSize.y);
    }

    ComponentCamera* camera = nullptr;
    
    if (app.GetPlayState() == Application::PlayState::PLAYING) {
        // En Play mode: usar la cámara de escena (ya cacheada)
        camera = app.camera->GetSceneCamera();
        
        // Fallback a editor si no hay cámara de juego
        if (!camera) {
            camera = app.camera->GetEditorCamera();
        }
    } else {
        // En Editor mode: usar cámara del editor
        camera = app.camera->GetEditorCamera();
    }

    if (!camera) {
        LOG_CONSOLE("[Lua] ERROR: No camera available");
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    glm::vec3 rayDir = camera->ScreenToWorldRay(mouseX, mouseY, screenWidth, screenHeight);
    glm::vec3 rayOrigin = camera->GetPosition();

    // Calculate intersection with plane Y = planeY
    if (std::abs(rayDir.y) < 0.0001f) {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    float t = (planeY - rayOrigin.y) / rayDir.y;

    if (t < 0) {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    glm::vec3 intersection = rayOrigin + t * rayDir;

    lua_pushnumber(L, intersection.x);
    lua_pushnumber(L, intersection.z);
    return 2;
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

    // Camera
    lua_newtable(L);
    lua_pushcfunction(L, Lua_Camera_GetScreenToWorldPlane);
    lua_setfield(L, -2, "GetScreenToWorldPlane");
    lua_setglobal(L, "Camera");

    LOG_CONSOLE("[ScriptManager] Engine functions registered: Engine, Input, Time, Camera");
}
// GAMEOBJECT API


// GameObject.Create(name) - Deferred operation
static int Lua_GameObject_Create(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    // Create userdata that will be filled in PostUpdate
    GameObject** udata = static_cast<GameObject**>(lua_newuserdata(L, sizeof(GameObject*)));
    *udata = nullptr;  // Temporarily null

    // Assign metatable
    luaL_getmetatable(L, "GameObject");
    lua_setmetatable(L, -2);

    // Enqueue the real creation for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([name, udata]() {
        GameObject* obj = Application::GetInstance().scene->CreateGameObject(name);

        if (obj) {
            GameObject* root = Application::GetInstance().scene->GetRoot();
            if (root && obj->GetParent() == nullptr) {
                root->AddChild(obj);
            }

            *udata = obj;  // Assign the created GameObject
        }
        });

    return 1;
}

// GameObject.Destroy(obj) - Deferred operation
static int Lua_GameObject_Destroy(lua_State* L) {
    GameObject** udata = static_cast<GameObject**>(luaL_checkudata(L, 1, "GameObject"));

    if (!udata || !*udata) {
        LOG_CONSOLE("[Lua] ERROR: Invalid GameObject in Destroy()");
        return 0;
    }

    GameObject* obj = *udata;

    // Enqueue destruction for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([obj]() {
        obj->MarkForDeletion();
        });

    return 0;
}

static int Lua_GameObject_Find(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    GameObject* root = Application::GetInstance().scene->GetRoot();
    if (!root) {
        lua_pushnil(L);
        return 1;
    }

    std::function<GameObject* (GameObject*, const std::string&)> findByName;
    findByName = [&](GameObject* node, const std::string& targetName) -> GameObject* {
        if (node->GetName() == targetName) {
            return node;
        }

        for (GameObject* child : node->GetChildren()) {
            GameObject* result = findByName(child, targetName);
            if (result) return result;
        }

        return nullptr;
        };

    GameObject* found = findByName(root, name);

    if (!found) {
        lua_pushnil(L);
        return 1;
    }

    GameObject** udata = static_cast<GameObject**>(lua_newuserdata(L, sizeof(GameObject*)));
    *udata = found;

    luaL_getmetatable(L, "GameObject");
    lua_setmetatable(L, -2);

    return 1;
}

// Helper functions for GameObject methods
static int Lua_GameObject_SetActive(lua_State* L) {
    GameObject** objPtr = static_cast<GameObject**>(luaL_checkudata(L, 1, "GameObject"));

    if (!objPtr || !*objPtr || (*objPtr)->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] ERROR: Cannot SetActive on invalid/deleted GameObject");
        return 0;
    }

    GameObject* obj = *objPtr;
    bool active = lua_toboolean(L, 2) != 0;

    // Enqueue state change for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([obj, active]() {
        if (!obj->IsMarkedForDeletion()) {
            obj->SetActive(active);
        }
        });

    return 0;
}

static int Lua_GameObject_IsActive(lua_State* L) {
    GameObject** objPtr = static_cast<GameObject**>(luaL_checkudata(L, 1, "GameObject"));

    if (!objPtr || !*objPtr || (*objPtr)->IsMarkedForDeletion()) {
        lua_pushboolean(L, false);
        return 1;
    }

    GameObject* obj = *objPtr;
    lua_pushboolean(L, obj->IsActive());
    return 1;
}

static int Lua_GameObject_AddComponent_MeshRenderer(lua_State* L) {
    GameObject** objPtr = static_cast<GameObject**>(luaL_checkudata(L, 1, "GameObject"));

    if (!objPtr || !*objPtr || (*objPtr)->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] ERROR: Cannot add component to invalid/deleted GameObject");
        lua_pushboolean(L, false);
        return 1;
    }

    GameObject* obj = *objPtr;

    // Enqueue component addition for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([obj]() {
        if (!obj->IsMarkedForDeletion()) {
            obj->CreateComponent(ComponentType::MESH); 
        }
        });

    lua_pushboolean(L, true);
    return 1;
}

static int Lua_GameObject_AddComponent_Material(lua_State* L) {
    GameObject** objPtr = static_cast<GameObject**>(luaL_checkudata(L, 1, "GameObject"));

    if (!objPtr || !*objPtr || (*objPtr)->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] ERROR: Cannot add component to invalid/deleted GameObject");
        lua_pushboolean(L, false);
        return 1;
    }

    GameObject* obj = *objPtr;

    // Enqueue component addition for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([obj]() {
        if (!obj->IsMarkedForDeletion()) {
            obj->CreateComponent(ComponentType::MATERIAL); 
        }
        });

    lua_pushboolean(L, true);
    return 1;
}

static int Lua_GameObject_AddComponent(lua_State* L) {
    GameObject* obj = *static_cast<GameObject**>(luaL_checkudata(L, 1, "GameObject"));
    const char* componentType = luaL_checkstring(L, 2);

    if (strcmp(componentType, "MeshRenderer") == 0) {
        return Lua_GameObject_AddComponent_MeshRenderer(L);
    }

    if (strcmp(componentType, "Material") == 0) {
        return Lua_GameObject_AddComponent_Material(L);
    }

    lua_pushboolean(L, false);
    return 1;
}

static int Lua_GameObject_LoadMesh(lua_State* L) {
    GameObject** objPtr = static_cast<GameObject**>(luaL_checkudata(L, 1, "GameObject"));

    if (!objPtr || !*objPtr || (*objPtr)->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] ERROR: Cannot load mesh on invalid/deleted GameObject");
        lua_pushboolean(L, false);
        return 1;
    }

    GameObject* obj = *objPtr;
    UID meshUID = static_cast<UID>(luaL_checknumber(L, 2));

    Component* comp = obj->GetComponent(ComponentType::MESH);
    ComponentMesh* mesh = static_cast<ComponentMesh*>(comp);
    if (!mesh) {
        lua_pushboolean(L, false);
        return 1;
    }

    // Enqueue mesh load for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([mesh, meshUID]() {
        mesh->LoadMeshByUID(meshUID);
        });

    lua_pushboolean(L, true);
    return 1;
}

static int Lua_GameObject_LoadTexture(lua_State* L) {
    GameObject** objPtr = static_cast<GameObject**>(luaL_checkudata(L, 1, "GameObject"));

    if (!objPtr || !*objPtr || (*objPtr)->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] ERROR: Cannot load texture on invalid/deleted GameObject");
        lua_pushboolean(L, false);
        return 1;
    }

    GameObject* obj = *objPtr;
    UID textureUID = static_cast<UID>(luaL_checknumber(L, 2));

    Component* comp = obj->GetComponent(ComponentType::MATERIAL);
    ComponentMaterial* mat = static_cast<ComponentMaterial*>(comp);
    if (!mat) {
        lua_pushboolean(L, false);
        return 1;
    }

    // Enqueue texture load for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([mat, textureUID]() {
        mat->LoadTextureByUID(textureUID);
        });

    lua_pushboolean(L, true);
    return 1;
}

// Helper for ComponentMesh.SetMesh
static int Lua_ComponentMesh_SetMesh(lua_State* L) {
    ComponentMesh* mesh = static_cast<ComponentMesh*>(lua_touserdata(L, lua_upvalueindex(1)));
    UID meshUID = static_cast<UID>(luaL_checknumber(L, 1));

    // Enqueue mesh change for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([mesh, meshUID]() {
        mesh->LoadMeshByUID(meshUID);
        });

    return 0;
}

// Helper for ComponentMaterial.SetTexture
static int Lua_ComponentMaterial_SetTexture(lua_State* L) {
    ComponentMaterial* mat = static_cast<ComponentMaterial*>(lua_touserdata(L, lua_upvalueindex(1)));
    UID textureUID = static_cast<UID>(luaL_checknumber(L, 1));

    // Enqueue texture change for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([mat, textureUID]() {
        mat->LoadTextureByUID(textureUID);
        });

    return 0;
}

static int Lua_GameObject_GetComponent(lua_State* L) {
    GameObject** objPtr = static_cast<GameObject**>(luaL_checkudata(L, 1, "GameObject"));

    if (!objPtr || !*objPtr || (*objPtr)->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] ERROR: Cannot get component from invalid/deleted GameObject");
        lua_pushnil(L);
        return 1;
    }

    GameObject* obj = *objPtr;
    const char* componentType = luaL_checkstring(L, 2);

    if (strcmp(componentType, "MeshRenderer") == 0) {
        Component* comp = obj->GetComponent(ComponentType::MESH);
        ComponentMesh* mesh = static_cast<ComponentMesh*>(comp);
        if (!mesh) {
            lua_pushnil(L);
            return 1;
        }

        lua_newtable(L);

        lua_pushlightuserdata(L, mesh);
        lua_pushcclosure(L, Lua_ComponentMesh_SetMesh, 1);
        lua_setfield(L, -2, "SetMesh");

        return 1;
    }

    if (strcmp(componentType, "Material") == 0) {
        Component* comp = obj->GetComponent(ComponentType::MATERIAL);
        ComponentMaterial* mat = static_cast<ComponentMaterial*>(comp);
        if (!mat) {
            lua_pushnil(L);
            return 1;
        }

        lua_newtable(L);

        lua_pushlightuserdata(L, mat);
        lua_pushcclosure(L, Lua_ComponentMaterial_SetTexture, 1);
        lua_setfield(L, -2, "SetTexture");

        return 1;
    }

    lua_pushnil(L);
    return 1;
}

static int Lua_GameObject_Index(lua_State* L) {
    GameObject** objPtr = static_cast<GameObject**>(luaL_checkudata(L, 1, "GameObject"));

    if (!objPtr || !*objPtr) {
        LOG_CONSOLE("[Lua] ERROR: Attempting to access invalid GameObject (null or deleted)");
        lua_pushnil(L);
        return 1;
    }

    GameObject* obj = *objPtr;

    if (obj->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] WARNING: Accessing GameObject marked for deletion: %s", obj->GetName().c_str());
        lua_pushnil(L);
        return 1;
    }

    const char* key = luaL_checkstring(L, 2);

    if (strcmp(key, "name") == 0) {
        lua_pushstring(L, obj->GetName().c_str());
        return 1;
    }

    if (strcmp(key, "transform") == 0) {
        Component* comp = obj->GetComponent(ComponentType::TRANSFORM);
        Transform* t = static_cast<Transform*>(comp);

        if (!t) {
            lua_pushnil(L);
            return 1;
        }

        Transform** udata = static_cast<Transform**>(lua_newuserdata(L, sizeof(Transform*)));
        *udata = t;

        luaL_getmetatable(L, "Transform");
        lua_setmetatable(L, -2);

        return 1;
    }

    if (strcmp(key, "SetActive") == 0) {
        lua_pushcfunction(L, Lua_GameObject_SetActive);
        return 1;
    }

    if (strcmp(key, "IsActive") == 0) {
        lua_pushcfunction(L, Lua_GameObject_IsActive);
        return 1;
    }

    if (strcmp(key, "GetComponent") == 0) {
        lua_pushcfunction(L, Lua_GameObject_GetComponent);
        return 1;
    }

    if (strcmp(key, "AddComponent") == 0) {
        lua_pushcfunction(L, Lua_GameObject_AddComponent);
        return 1;
    }

    if (strcmp(key, "LoadMesh") == 0) {
        lua_pushcfunction(L, Lua_GameObject_LoadMesh);
        return 1;
    }

    if (strcmp(key, "LoadTexture") == 0) {
        lua_pushcfunction(L, Lua_GameObject_LoadTexture);
        return 1;
    }

    return 0;
}

void ScriptManager::RegisterGameObjectAPI() {
    // Create metatable for GameObject
    luaL_newmetatable(L, "GameObject");

    lua_pushcfunction(L, Lua_GameObject_Index);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);

    // Create global GameObject table
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

// TRANSFORM API

// Helper functions for Transform methods
static int Lua_Transform_SetPosition(lua_State* L) {
    Transform** tPtr = static_cast<Transform**>(luaL_checkudata(L, 1, "Transform"));

    if (!tPtr || !*tPtr) {
        LOG_CONSOLE("[Lua] ERROR: Cannot set position on invalid Transform");
        return 0;
    }

    Transform* t = *tPtr;

    // Verificar si el GameObject propietario está marcado para eliminación
    GameObject* owner = t->GetOwner();
    if (owner && owner->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] WARNING: Attempting to set position on deleted GameObject");
        return 0;
    }

    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float z = static_cast<float>(luaL_checknumber(L, 4));

    // Enqueue position change for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([t, x, y, z]() {
        GameObject* owner = t->GetOwner();
        if (owner && !owner->IsMarkedForDeletion()) {
            t->SetPosition(glm::vec3(x, y, z));
        }
        });

    return 0;
}

static int Lua_Transform_SetRotation(lua_State* L) {
    Transform** tPtr = static_cast<Transform**>(luaL_checkudata(L, 1, "Transform"));

    if (!tPtr || !*tPtr) {
        LOG_CONSOLE("[Lua] ERROR: Cannot set rotation on invalid Transform");
        return 0;
    }

    Transform* t = *tPtr;

    // Verificar si el GameObject propietario está marcado para eliminación
    GameObject* owner = t->GetOwner();
    if (owner && owner->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] WARNING: Attempting to set rotation on deleted GameObject");
        return 0;
    }

    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float z = static_cast<float>(luaL_checknumber(L, 4));

    // Enqueue rotation change for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([t, x, y, z]() {
        GameObject* owner = t->GetOwner();
        if (owner && !owner->IsMarkedForDeletion()) {
            t->SetRotation(glm::vec3(x, y, z));
        }
        });

    return 0;
}

static int Lua_Transform_SetScale(lua_State* L) {
    Transform** tPtr = static_cast<Transform**>(luaL_checkudata(L, 1, "Transform"));

    if (!tPtr || !*tPtr) {
        LOG_CONSOLE("[Lua] ERROR: Cannot set scale on invalid Transform");
        return 0;
    }

    Transform* t = *tPtr;

    // Verificar si el GameObject propietario está marcado para eliminación
    GameObject* owner = t->GetOwner();
    if (owner && owner->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] WARNING: Attempting to set scale on deleted GameObject");
        return 0;
    }

    float x = static_cast<float>(luaL_checknumber(L, 2));
    float y = static_cast<float>(luaL_checknumber(L, 3));
    float z = static_cast<float>(luaL_checknumber(L, 4));

    // Enqueue scale change for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([t, x, y, z]() {
        GameObject* owner = t->GetOwner();
        if (owner && !owner->IsMarkedForDeletion()) {
            t->SetScale(glm::vec3(x, y, z));
        }
        });

    return 0;
}

static int Lua_Transform_Index(lua_State* L) {
    Transform** tPtr = static_cast<Transform**>(luaL_checkudata(L, 1, "Transform"));

    if (!tPtr || !*tPtr) {
        LOG_CONSOLE("[Lua] ERROR: Accessing invalid Transform");
        lua_pushnil(L);
        return 1;
    }

    Transform* t = *tPtr;

    // Verificar si el GameObject propietario está marcado para eliminación
    GameObject* owner = t->GetOwner();
    if (owner && owner->IsMarkedForDeletion()) {
        LOG_CONSOLE("[Lua] WARNING: Accessing Transform of deleted GameObject");
        lua_pushnil(L);
        return 1;
    }

    const char* key = luaL_checkstring(L, 2);

    // LOCAL POSITION
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

    if (strcmp(key, "worldPosition") == 0) {
        try {
            // Intentar obtener la matriz global
            const glm::mat4& worldMatrix = t->GetGlobalMatrix();

            // Extraer posición mundial - GLM matrices son column-major
            // La última columna (índice 3) contiene la posición
            float worldX = worldMatrix[3][0];
            float worldY = worldMatrix[3][1];
            float worldZ = worldMatrix[3][2];


            // Crear tabla Lua
            lua_newtable(L);
            lua_pushnumber(L, worldX);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, worldY);
            lua_setfield(L, -2, "y");
            lua_pushnumber(L, worldZ);
            lua_setfield(L, -2, "z");

            return 1;
        }
        catch (const std::exception& e) {
            LOG_CONSOLE("[Lua] ERROR getting worldPosition: %s", e.what());
            lua_pushnil(L);
            return 1;
        }
        catch (...) {
            LOG_CONSOLE("[Lua] ERROR: Unknown exception getting worldPosition");
            lua_pushnil(L);
            return 1;
        }
    }

    // ROTATION
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

    // SCALE
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

    // METHODS
    if (strcmp(key, "SetPosition") == 0) {
        lua_pushcfunction(L, Lua_Transform_SetPosition);
        return 1;
    }

    if (strcmp(key, "SetRotation") == 0) {
        lua_pushcfunction(L, Lua_Transform_SetRotation);
        return 1;
    }

    if (strcmp(key, "SetScale") == 0) {
        lua_pushcfunction(L, Lua_Transform_SetScale);
        return 1;
    }

    // Si la clave no se encuentra, retornar nil
    lua_pushnil(L);
    return 1;
}
void ScriptManager::RegisterComponentAPI() {
    // Transform metatable
    luaL_newmetatable(L, "Transform");

    lua_pushcfunction(L, Lua_Transform_Index);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);

}

// PREFAB API

// Prefab.Load(name, filepath)
static int Lua_Prefab_Load(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    const char* filepath = luaL_checkstring(L, 2);

    bool success = PrefabManager::GetInstance().LoadPrefab(name, filepath);

    lua_pushboolean(L, success);
    return 1;
}

// Prefab.Instantiate(name) - Deferred operation
static int Lua_Prefab_Instantiate(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);

    // Create userdata that will be filled in PostUpdate
    GameObject** udata = static_cast<GameObject**>(lua_newuserdata(L, sizeof(GameObject*)));
    *udata = nullptr;  // Temporarily null

    luaL_getmetatable(L, "GameObject");
    lua_setmetatable(L, -2);

    // Enqueue instantiation for PostUpdate
    auto& app = Application::GetInstance();
    app.scripts->EnqueueOperation([name, udata]() {
        GameObject* instance = nullptr;

        // First check if prefab is already loaded in PrefabManager
        if (PrefabManager::GetInstance().HasPrefab(name)) {
            instance = PrefabManager::GetInstance().InstantiatePrefab(name);
        }
        else {
            // Try to find prefab in resources
            ModuleResources* resources = Application::GetInstance().resources.get();

            // Search for prefab by matching filename in all resources
            UID prefabUID = 0;
            const auto& allResources = resources->GetAllResources();

            for (const auto& pair : allResources) {
                UID uid = pair.first;
                Resource* res = pair.second;

                if (res->GetType() == Resource::PREFAB) {
                    std::string assetPath = res->GetAssetFile();

                    // Extract filename from path
                    size_t lastSlash = assetPath.find_last_of("/\\");
                    std::string filename = (lastSlash != std::string::npos)
                        ? assetPath.substr(lastSlash + 1)
                        : assetPath;

                    // Match either by filename or by exact path
                    if (filename == name || assetPath == name) {
                        prefabUID = uid;
                        break;
                    }
                }
            }

            if (prefabUID != 0) {
                // Request the prefab resource
                Resource* res = resources->RequestResource(prefabUID);
                if (res && res->GetType() == Resource::PREFAB) {
                    ResourcePrefab* prefabRes = static_cast<ResourcePrefab*>(res);
                    instance = prefabRes->Instantiate();

                    if (instance) {
                        // Enable all scripts in the instantiated object and its children
                        std::function<void(GameObject*)> enableScripts = [&](GameObject* obj) {
                            // Get all script components and call their Start()
                            auto components = obj->GetComponents();
                            for (auto* comp : components) {
                                if (comp->GetType() == ComponentType::SCRIPT) {
                                    ComponentScript* script = static_cast<ComponentScript*>(comp);
                                    if (script->IsActive()) {
                                        script->CallStart();
                                    }
                                }
                            }

                            // Recursively enable scripts in children
                            for (GameObject* child : obj->GetChildren()) {
                                enableScripts(child);
                            }
                            };

                        enableScripts(instance);
                    }
                }
            }
        }

        if (instance) {
            *udata = instance;
        }
        else {
            LOG_CONSOLE("[Lua] ERROR: Failed to instantiate prefab: %s", name);
        }
        });

    return 1;
}

void ScriptManager::RegisterPrefabAPI() {
    lua_newtable(L);

    lua_pushcfunction(L, Lua_Prefab_Load);
    lua_setfield(L, -2, "Load");

    lua_pushcfunction(L, Lua_Prefab_Instantiate);
    lua_setfield(L, -2, "Instantiate");

    lua_setglobal(L, "Prefab");

}

static GameWindow* GetGameWindow() {
    GameWindow* window = Application::GetInstance().editor->GetGameWindow();
    return window;
}