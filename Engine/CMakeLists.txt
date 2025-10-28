cmake_minimum_required(VERSION 3.10)

project(Engine)

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(DevIL REQUIRED)
find_package(imgui REQUIRED)

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
)

set(UTILS_SRC 
    src/Globals.h 
    src/Log.cpp 
    src/Log.h 
    src/Time.cpp 
    src/Time.h 
    src/FileSystem.cpp 
    src/FileSystem.h 
    src/Texture.h
    src/Texture.cpp
    src/Grid.cpp
    src/Grid.h
    src/ModuleEditor.cpp
    src/ModuleEditor.h
)

set(GAMEOBJECTS_SRC 
    src/Component.h  
    src/Component.cpp 
    src/ComponentMaterial.cpp 
    src/ComponentMaterial.h  
    src/ComponentMesh.h  
    src/ComponentMesh.cpp 
    src/GameObject.cpp 
    src/GameObject.h 
    src/ModuleScene.cpp 
    src/ModuleScene.h 
    src/Transform.cpp 
    src/Transform.h 
)

source_group("Source\\Core" FILES ${CORE_SRC})
source_group("Source\\Rendering" FILES ${RENDERING_SRC})
source_group("Source\\Utils" FILES ${UTILS_SRC})
source_group("Source\\Game Objects" FILES ${GAMEOBJECTS_SRC})

set(SRCS ${CORE_SRC} ${RENDERING_SRC} ${UTILS_SRC} ${GAMEOBJECTS_SRC})

add_executable(Engine ${SRCS})

target_link_libraries(Engine PRIVATE SDL3::SDL3)
target_link_libraries(Engine PRIVATE glad::glad)
target_link_libraries(Engine PRIVATE glm::glm)
target_link_libraries(Engine PRIVATE assimp::assimp)
target_link_libraries(Engine PRIVATE DevIL::IL)
target_link_libraries(Engine PRIVATE DevIL::ILU)
target_link_libraries(Engine PRIVATE imgui::imgui)