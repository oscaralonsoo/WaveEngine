# 🌊 Wave Engine

> **A Custom 3D Game Engine built with C++ and OpenGL**

Wave Engine is a feature-rich game engine developed as part of the Game Engines course at CITM-UPC. This delivery (E3) focuses on implementing a complete shader pipeline with advanced rendering techniques including Blinn-Phong lighting, procedural water simulation, and transparent object rendering.

**Technologies:** C++17 | OpenGL 4.6 | GLSL 330 | ImGui | Assimp | DevIL  
**Course:** Game Engines (Motores de Videojuegos)  
**Institution:** CITM - Universitat Politècnica de Catalunya  
**Delivery:** E3 - Shader Pipeline

**Repository:** [bottzo/Motor2025](https://github.com/bottzo/Motor2025)  
**Branch:** [Shaders_EduPau-G12](https://github.com/bottzo/Motor2025/tree/Shaders_EduPau-G12)  
**Releases:** [Download Latest](https://github.com/bottzo/Motor2025/releases)

---

## Table of Contents

- [Team](#-team)
- [Shader System](#-shader-system)
- [Controls](#-controls)
- [Features](#-features)
- [Demo](#-demo)
- [License](#-license)

---

## Team

| GitHub | Role | Contributions |
|--------|------|---------------|
| [PauGutsens](https://github.com/PauGutsens) | Lead Programmer | Shader editor with hot-reloading, Blinn-Phong lighting implementation, transparency rendering system |
| [eduuu04](https://github.com/eduuu04) | Lead Programmer | Procedural water shader with Gerstner waves, editable shader uniforms, debug visualization system |

---

## Shader System

### Overview

The engine implements a complete shader pipeline with real-time editing capabilities, physically-based lighting, and advanced rendering techniques. All shaders are managed as resources with unique identifiers and support hot-reloading for rapid iteration.

### Shader Editor & Hot-Reloading

**Features:**
- Integrated GLSL code editor with syntax highlighting
- Real-time compilation on save (Ctrl+S)
- Automatic shader reloading without engine restart
- Comprehensive error reporting in console and editor UI
- Resource management system with UID tracking

**Implementation Details:**
```cpp
// ShaderEditorWindow.cpp - Hot-reload workflow
void CompileAndSave() {
    ResourceShader* shader = GetCurrentShader();
    shader->SetSourceCode(editorBuffer);
    
    if (shader->Compile()) {
        // Shader compiled successfully - changes applied immediately
        SaveToDisk();
    } else {
        // Display compilation errors in UI
        ShowErrorLog();
    }
}
```

### Blinn-Phong Lighting

**Lighting Model:**
- **Ambient:** Base illumination independent of light direction
- **Diffuse:** Lambertian reflectance based on surface normal
- **Specular:** Blinn-Phong highlights using halfway vector

**Rendering Modes:**
- **Per-Vertex (Gouraud):** Lighting calculated in vertex shader, interpolated across fragments
- **Per-Pixel (Blinn-Phong):** Full lighting calculation per fragment for smooth, accurate results

**Editable Parameters:**
- Diffuse color (vec4)
- Specular color (vec4)
- Shininess coefficient (float)
- Lighting mode toggle

**GLSL Implementation:**
```glsl
// Fragment Shader - Blinn-Phong Specular
vec3 viewDir = normalize(viewPos - FragPos);
vec3 lightDir = normalize(-lightDirection);
vec3 halfwayDir = normalize(lightDir + viewDir);

float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
vec3 specular = specularColor * spec;

vec3 result = ambient + diffuse + specular;
```

### Procedural Water Shader

**Architecture:** Three-stage pipeline (Vertex → Geometry → Fragment)

**Stage 1 - Vertex Shader:**
- Passes vertex data to geometry shader
- Minimal processing for maximum geometry shader flexibility

**Stage 2 - Geometry Shader:**
- **Gerstner Wave Simulation:** Physically-accurate ocean waves using multiple wave functions
- **Dynamic Tessellation:** Subdivides input triangles (1→4) for increased detail
- **Normal Calculation:** Derives surface normals from tangent and binormal vectors

**Stage 3 - Fragment Shader:**
- **Height-based Coloring:** Smooth gradient from deep blue → shallow blue → white foam
- **Specular Highlights:** Sun reflection on water surface
- **Fresnel Effect:** Angle-dependent transparency for realistic water appearance

**Editable Parameters:**
```glsl
uniform float waveSpeed;      // Animation speed (0-10)
uniform float waveAmplitude;  // Wave height (0-5)
uniform float waveFrequency;  // Wave density (0-10)
uniform float opacity;        // Water transparency (0-1)
```

**Wave Mathematics:**
```glsl
// Gerstner Wave Function
vec3 GerstnerWave(vec3 position, Wave wave, float time) {
    float k = 2.0 * PI / wave.wavelength;
    float c = sqrt(9.8 / k);  // Wave speed from gravity
    float f = k * (dot(wave.direction, position.xz) - c * time);
    float a = wave.steepness / k;
    
    return vec3(
        wave.direction.x * a * cos(f),
        a * sin(f),
        wave.direction.y * a * cos(f)
    );
}
```

### Transparency Rendering

**Rendering Pipeline:**
1. **Opaque Pass:** Render all solid objects with depth writing enabled
2. **Transparency Collection:** Gather all transparent objects with camera distance
3. **Depth Sorting:** Sort transparent objects back-to-front (painter's algorithm)
4. **Transparent Pass:** Render with alpha blending, depth writing disabled

**Implementation:**
```cpp
// Renderer.cpp - Transparency sorting
std::sort(transparentObjects.begin(), transparentObjects.end(),
    [](const TransparentObject& a, const TransparentObject& b) {
        return a.distanceToCamera > b.distanceToCamera;  // Back-to-front
    });

glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDepthMask(GL_FALSE);  // Disable depth writing for transparency
```

**Transparency Detection:**
- Objects with opacity < 1.0
- Materials with MaterialType::WATER
- Automatic sorting every frame

### Uniform Management

All shader parameters are exposed through the Inspector interface:

**Material Properties:**
- Color properties (Diffuse, Specular, Ambient, Emissive)
- Physical properties (Shininess, Opacity, Metallic, Roughness)
- Lighting mode selection (Vertex/Pixel)

**Water Properties:**
- Wave parameters (Speed, Amplitude, Frequency)
- Real-time preview in scene view

**Debug Features:**
- Compilation error display in editor
- Console logging for shader operations
- Visual feedback for shader changes

---

## 🎮 Controls

### Camera Navigation

| Action | Primary | Secondary |
|--------|---------|-----------|
| Move Up | `Space` | - |
| Move Down | `Left Ctrl` | - |
| Zoom | `Mouse Wheel` | - |
| Speed Boost | `Shift` (hold) | - |
| Free Look | `Right Mouse` + `WASD` | - |
| Orbit Object | `Left Alt` + `Left Mouse` | - |
| Focus Selection | `F` | - |

### Object Manipulation

| Action | Key |
|--------|-----|
| Select Object | `Left Mouse` |
| Multi-Select | `Shift` + `Left Mouse` |
| Delete Object | `Backspace` |

### Gizmo Operations

| Mode | Key |
|------|-----|
| Translate | `W` |
| Rotate | `E` |
| Scale | `R` |
| Toggle Local/World | `T` |

---

## Features

### Core Engine Systems

**Resource Management:**
- Custom binary formats for optimized loading (.wmesh, .wtex, .wscene)
- Asset pipeline with automatic Library generation
- Reference counting for efficient memory usage
- Hot-reloading support for all resource types

**Rendering Optimizations:**
- Frustum culling with configurable camera
- Octree spatial partitioning for scene acceleration
- Batch rendering for improved performance
- Debug visualization (AABBs, Octree, Frustum)

**Scene Management:**
- Hierarchical GameObject system with parent-child relationships
- JSON-based scene serialization
- Runtime object manipulation (transform, components)
- Automatic scene loading on startup

**Editor Interface:**
- **Console:** Event logging, resource operations, shader compilation feedback
- **Configuration:** FPS monitoring, window settings, renderer options, hardware info
- **Assets:** Resource browser with drag-and-drop import, metadata editing
- **Hierarchy:** Scene tree with drag-to-reparent functionality
- **Inspector:** Component editing, material properties, shader assignment
- **Shader Editor:** Real-time GLSL editing with error reporting

### Additional Features

- Alpha-tested textures for vegetation and cutouts
- Z-buffer visualization for depth debugging
- Customizable import settings (textures, meshes)
- Multi-object selection with visual feedback
- Configurable camera component with frustum visualization

---

## 🎬 Demo

### Editor Functionality
![Editor Demo](./docs/editor_demo.gif)
> Real-time shader editing, material assignment, and parameter adjustment

### In-Game Rendering
![Game Demo](./docs/game_demo.gif)
> City scene with Blinn-Phong lighting, animated water, and transparent objects

---

## License

**MIT License**

Copyright © 2025 Wave Engine - Pau Gutsens & Edu García

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

---

**Links:**
- [GitHub Repository](https://github.com/bottzo/Motor2025)
- [E3 Branch - Shaders_EduPau-G12](https://github.com/bottzo/Motor2025/tree/Shaders_EduPau-G12)
- [Latest Release](https://github.com/bottzo/Motor2025/releases)
- [MIT License](https://github.com/PauGutsens/Motor?tab=MIT-1-ov-file)


