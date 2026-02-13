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
    src/Shaders.h 
    src/Shaders.cpp 
    src/Frustum.h 
    src/Frustum.cpp 
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
)

source_group("Source\\Core" FILES ${CORE_SRC})
source_group("Source\\Rendering" FILES ${RENDERING_SRC})
source_group("Source\\Utils" FILES ${UTILS_SRC})
source_group("Source\\Utils\\Editor" FILES ${EDITOR_SRC})
source_group("Source\\Game Objects" FILES ${GAMEOBJECTS_SRC})
source_group("Source\\Game Objects\\Components" FILES ${COMPONENTS_SRC})
source_group("Source\\Loaders" FILES ${LOADERS_SRC})
source_group("Source\\Loaders\\Importers" FILES ${IMPORTERS_SRC})

set(SRCS ${CORE_SRC} ${RENDERING_SRC} ${UTILS_SRC} ${EDITOR_SRC} ${GAMEOBJECTS_SRC} ${COMPONENTS_SRC} ${LOADERS_SRC} ${IMPORTERS_SRC})

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




set(NOESIS_SDK_DIR "${CMAKE_SOURCE_DIR}/external/NoesisGUI")

# Definir archivos del renderizador (Ajusta la ruta según donde esté el SDK)
set(NOESIS_RENDERER_SRC 
    "${NOESIS_SDK_DIR}/Src/Packages/Render/GLRenderDevice/GLRenderDevice.cpp"
)

# Añadir a la compilación
target_sources(Engine PRIVATE ${NOESIS_RENDERER_SRC})
target_include_directories(Engine PRIVATE "${NOESIS_SDK_DIR}/Include")

if(WIN32)
    set(NOESIS_LIB "${NOESIS_SDK_DIR}/Lib/windows_x86_64/Noesis.lib")
    set(NOESIS_DLL "${NOESIS_SDK_DIR}/Bin/windows_x86_64/Noesis.dll")
    
    target_link_libraries(Engine PRIVATE ${NOESIS_LIB})
    
    # Copia automática de la DLL al generar el .exe
    add_custom_command(TARGET Engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${NOESIS_DLL}"
        $<TARGET_FILE_DIR:Engine>
    )
endif()