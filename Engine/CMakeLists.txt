cmake_minimum_required(VERSION 3.10)

project(Engine)

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(DevIL REQUIRED)
find_package(imgui REQUIRED)
find_package(imguizmo CONFIG REQUIRED)

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
    src/ModuleEditor.cpp
    src/ModuleEditor.h
    src/SelectionManager.h
    src/SelectionManager.cpp
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

)

set(LOADERS_SRC 
    src/Texture.h 
    src/Texture.cpp  
    src/FileSystem.h  
    src/FileSystem.cpp 
)

source_group("Source\\Core" FILES ${CORE_SRC})
source_group("Source\\Rendering" FILES ${RENDERING_SRC})
source_group("Source\\Utils" FILES ${UTILS_SRC})
source_group("Source\\Game Objects" FILES ${GAMEOBJECTS_SRC})
source_group("Source\\Game Objects\\Components" FILES ${COMPONENTS_SRC})
source_group("Source\\Loaders" FILES ${LOADERS_SRC})

set(SRCS ${CORE_SRC} ${RENDERING_SRC} ${UTILS_SRC} ${GAMEOBJECTS_SRC} ${COMPONENTS_SRC} ${LOADERS_SRC})

add_executable(Engine ${SRCS})

target_link_libraries(Engine PRIVATE SDL3::SDL3)
target_link_libraries(Engine PRIVATE glad::glad)
target_link_libraries(Engine PRIVATE glm::glm)
target_link_libraries(Engine PRIVATE assimp::assimp)
target_link_libraries(Engine PRIVATE DevIL::IL)
target_link_libraries(Engine PRIVATE DevIL::ILU)
target_link_libraries(Engine PRIVATE imgui::imgui)
target_link_libraries(Engine PRIVATE imguizmo::imguizmo)