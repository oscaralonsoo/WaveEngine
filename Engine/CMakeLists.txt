cmake_minimum_required(VERSION 3.10)

project(Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(DevIL REQUIRED)
find_package(imgui REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)

set(CORE_SRC 
    src/Main.cpp 
    src/Application.cpp
    src/Application.h
    src/Input.cpp
    src/Input.h 
    src/Module.h 
    src/Window.cpp 
    src/Window.h
)

set(RENDERING_SRC 
    src/Camera.h 
    src/Camera.cpp
    src/Primitives.cpp 
    src/Primitives.h 
    src/RenderContext.h 
    src/RenderContext.cpp 
    src/Renderer.h 
    src/Renderer.cpp 
    src/Shader.h 
    src/Shader.cpp 
    src/Frustum.h 
    src/Frustum.cpp 
    src/UI.h
    src/UI.cpp 
)

set(VFX_SRC
    src/ParticleSystem.h
    src/ParticleSystem.cpp
)

set(PHYSICS_SRC
    src/ModulePhysics.h
    src/ModulePhysics.cpp
    src/Rigidbody.cpp
    src/Rigidbody.h
    src/Collider.cpp
    src/Collider.h
    src/BoxCollider.cpp
    src/BoxCollider.h
    src/SphereCollider.h
    src/SphereCollider.cpp
    src/CapsuleCollider.h
    src/CapsuleCollider.cpp
    src/ConvexCollider.h
    src/ConvexCollider.cpp
    src/InfinitePlaneCollider.h
    src/InfinitePlaneCollider.cpp
    src/PlaneCollider.cpp
    src/PlaneCollider.h
    src/MeshCollider.h
    src/MeshCollider.cpp
    src/Joint.cpp
    src/Joint.h
    src/SphericalJoint.cpp
    src/SphericalJoint.h
    src/PrismaticJoint.cpp
    src/PrismaticJoint.h
    src/HingeJoint.cpp
    src/HingeJoint.h
    src/FixedJoint.cpp
    src/FixedJoint.h
    src/DistanceJoint.cpp
    src/DistanceJoint.h
    src/D6Joint.cpp
    src/D6Joint.h
    src/PhysicsEventsListener.h
    src/PhysicsCooker.h
    src/PhysicsCooker.cpp
)

set(UTILS_SRC 
    src/Globals.h 
    src/Log.cpp 
    src/Log.h 
    src/Time.cpp 
    src/Time.h 
    src/Grid.cpp
    src/Grid.h
    src/ModuleCamera.cpp 
    src/ModuleCamera.h 
    src/SelectionManager.h
    src/SelectionManager.cpp
    src/Octree.h
    src/Octree.cpp
)

set(GAMEOBJECTS_SRC 
    src/GameObject.cpp 
    src/GameObject.h 
    src/ModuleScene.cpp 
    src/ModuleScene.h 
)

set(COMPONENTS_SRC
    src/Component.h
    src/Component.cpp
    src/ComponentMaterial.cpp
    src/ComponentMaterial.h
    src/ComponentMesh.h
    src/ComponentMesh.cpp
    src/Transform.cpp
    src/Transform.h
    src/ComponentCamera.cpp
    src/ComponentCamera.h
    src/ComponentRotate.cpp
    src/ComponentRotate.h
    src/ComponentParticleSystem.h
    src/ComponentParticleSystem.cpp
)

set(LOADERS_SRC 
    src/Texture.h 
    src/Texture.cpp  
    src/FileSystem.h  
    src/FileSystem.cpp 
)

set(EDITOR_SRC 
    src/ModuleEditor.h
    src/ModuleEditor.cpp 
    src/SceneWindow.h
    src/SceneWindow.cpp 
    src/EditorWindow.h 
    src/ConfigurationWindow.h
    src/ConfigurationWindow.cpp 
    src/ConsoleWindow.h
    src/ConsoleWindow.cpp 
    src/HierarchyWindow.h
    src/HierarchyWindow.cpp 
    src/InspectorWindow.h
    src/InspectorWindow.cpp 
    src/GameWindow.h
    src/GameWindow.cpp
    src/AssetsWindow.h
    src/AssetsWindow.cpp 
    src/ImportSettingsWindow.h
    src/ImportSettingsWindow.cpp 
    src/ShaderEditorWindow.h
    src/ShaderEditorWindow.cpp
)

set(IMPORTERS_SRC 
    src/MeshImporter.cpp 
    src/MeshImporter.h 
    src/LibraryManager.cpp 
    src/LibraryManager.h 
    src/MetaFile.cpp 
    src/MetaFile.h 
    src/TextureImporter.cpp 
    src/TextureImporter.h 
    src/ModuleResources.cpp 
    src/ModuleResources.h 
    src/ResourceMesh.cpp 
    src/ResourceMesh.h 
    src/ResourceTexture.cpp 
    src/ResourceTexture.h 
    src/ResourceShader.cpp
    src/ResourceShader.h
)

set(SCRIPTING_SRC 
    src/ScriptManager.h
    src/ScriptManager.cpp
    src/ResourceScript.h
    src/ResourceScript.cpp
    src/ComponentScript.h
    src/ComponentScript.cpp
    src/ScriptEditorWindow.h
    src/ScriptEditorWindow.cpp
    src/EditorPreferences.h
    src/EditorPreferences.cpp
    src/Prefab.h
    src/Prefab.cpp
    src/PrefabManager.h
    src/PrefabManager.cpp
    src/ResourcePrefab.h
    src/ResourcePrefab.cpp
    src/TextEditor.h
    src/TextEditor.cpp
)

source_group("Source\\Core" FILES ${CORE_SRC})
source_group("Source\\Rendering" FILES ${RENDERING_SRC})
source_group("Source\\Rendering\\VFX" FILES ${VFX_SRC})
source_group("Source\\Utils" FILES ${UTILS_SRC})
source_group("Source\\Utils\\Editor" FILES ${EDITOR_SRC})
source_group("Source\\Game Objects" FILES ${GAMEOBJECTS_SRC})
source_group("Source\\Game Objects\\Components" FILES ${COMPONENTS_SRC})
source_group("Source\\Loaders" FILES ${LOADERS_SRC})
source_group("Source\\Loaders\\Importers" FILES ${IMPORTERS_SRC})
source_group("Scripting" FILES ${SCRIPTING_SRC})
source_group("Source\\Physics" FILES ${PHYSICS_SRC})

set(SRCS ${CORE_SRC} ${RENDERING_SRC} ${VFX_SRC} ${UTILS_SRC} ${EDITOR_SRC} ${GAMEOBJECTS_SRC} ${COMPONENTS_SRC} ${LOADERS_SRC} ${IMPORTERS_SRC} ${SCRIPTING_SRC} ${PHYSICS_SRC})

add_executable(Engine ${SRCS})

target_link_libraries(Engine PRIVATE SDL3::SDL3)
target_link_libraries(Engine PRIVATE glad::glad)
target_link_libraries(Engine PRIVATE glm::glm)
target_link_libraries(Engine PRIVATE assimp::assimp)
target_link_libraries(Engine PRIVATE DevIL::IL)
target_link_libraries(Engine PRIVATE DevIL::ILU)
target_link_libraries(Engine PRIVATE imgui::imgui)
target_link_libraries(Engine PRIVATE imguizmo::imguizmo)
target_link_libraries(Engine PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(Engine PRIVATE Tracy::TracyClient)
target_link_libraries(Engine PRIVATE unofficial::omniverse-physx-sdk::sdk)
target_link_libraries(Engine PRIVATE ${LUA_LIBRARIES})

target_include_directories(Engine PRIVATE ${LUA_INCLUDE_DIR})

# ---- NoesisGUI ----
set(NOESIS_SDK_DIR "${CMAKE_SOURCE_DIR}/External/NoesisGUI")

# --- Sources: glob por paquete completo ---
file(GLOB NOESIS_APP_PROVIDERS_SOURCES
    "${NOESIS_SDK_DIR}/Src/Packages/App/Providers/Src/*.cpp"
)
file(GLOB NOESIS_RENDER_SOURCES
    "${NOESIS_SDK_DIR}/Src/Packages/Render/GLRenderDevice/Src/*.cpp"
)
file(GLOB NOESIS_INTERACTIVITY_SOURCES
    "${NOESIS_SDK_DIR}/Src/Packages/App/Interactivity/Src/*.cpp"
)
file(GLOB NOESIS_MEDIAELEMENT_SOURCES
    "${NOESIS_SDK_DIR}/Src/Packages/App/MediaElement/Src/*.cpp"
)

target_sources(Engine PRIVATE
    ${NOESIS_APP_PROVIDERS_SOURCES}
    ${NOESIS_RENDER_SOURCES}
    ${NOESIS_INTERACTIVITY_SOURCES}
    ${NOESIS_MEDIAELEMENT_SOURCES}
)

# --- Includes: todos los paquetes de golpe ---
file(GLOB NOESIS_PACKAGE_INCLUDE_DIRS
    LIST_DIRECTORIES true
    "${NOESIS_SDK_DIR}/Src/Packages/*/Include"
    "${NOESIS_SDK_DIR}/Src/Packages/*/*/Include"
    "${NOESIS_SDK_DIR}/Src/Packages/*/Src"
    "${NOESIS_SDK_DIR}/Src/Packages/*/*/Src"
)
target_include_directories(Engine PRIVATE
    "${NOESIS_SDK_DIR}/Include"
    ${NOESIS_PACKAGE_INCLUDE_DIRS}
)

# --- Macros de compilación estática ---
target_compile_definitions(Engine PRIVATE
    NS_APP_PROVIDERS_API=
    NS_APP_FRAMEWORK_API=
    NS_APP_INTERACTIVITY_API=
    NS_RENDER_GLRENDERDEVICE_API=
    NS_APP_MEDIAELEMENT_API=
)

if(MSVC)
    target_compile_options(Engine PRIVATE /FS)
endif()
if(WIN32)
    set(NOESIS_LIB "${NOESIS_SDK_DIR}/Lib/windows_x86_64/Noesis.lib")
    set(NOESIS_DLL "${NOESIS_SDK_DIR}/Bin/windows_x86_64/Noesis.dll")

    target_link_libraries(Engine PRIVATE ${NOESIS_LIB})

    add_custom_command(TARGET Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${NOESIS_DLL}"
        $<TARGET_FILE_DIR:Engine>
    )
endif()

