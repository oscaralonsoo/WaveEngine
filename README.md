# 🌊 Wave Engine

> **A Custom 3D Game Engine built with C++ and OpenGL**

Wave Engine is a feature-rich game engine developed as part of the Game Engines course at CITM-UPC. This project integrates several external libraries including Assimp (model loading), DevIL (textures), ImGui (UI), Lua (scripting), and NVIDIA PhysX (physics).

The engine focuses on modularity and high-performance rendering, featuring a complete shader pipeline (Blinn-Phong, Gerstner waves), a robust resource management system, and a Lua scripting system for runtime behavior.

**Technologies:** C++17 | OpenGL 4.6 | GLSL 330 | ImGui | Assimp | DevIL | Lua | PhysX | tracy

---

## 📹 Video Demonstration

[![Ver video](https://img.youtube.com/vi/79wXfoxlBjI/maxresdefault.jpg)](https://www.youtube.com/watch?v=79wXfoxlBjI)

<p align="center"><em>Click the image above to watch the full engine demonstration video</em></p>

---

## 🏗️ Core Engine Systems

### **Application**
The core module that manages the engine's lifecycle (Awake → Start → Update → PostUpdate → CleanUp). Handles Play/Pause/Stop states.

### **ModuleScene**
Manages the scene and GameObject tree. Maintains an Octree for spatial optimization (frustum culling, raycasting).

### **GameObject / Components**
Entity-component system:
- **Transform:** Position, rotation, scale
- **ComponentMesh:** 3D geometry
- **ComponentMaterial:** Textures and materials (Standard, Water, etc.)
- **ComponentCamera:** Game and Scene cameras
- **ComponentScript:** Lua scripts for custom behavior
- **ComponentParticleSystem:** Particle effects
- **Physics Components:** Rigidbody and Various Colliders (Box, Sphere, Capsule, Mesh)

### **ModuleResources**
Resource system with unique UIDs. Manages:
- **ResourceMesh** / **ResourceTexture** / **ResourceScript** / **ResourcePrefab** / **ResourceShader**

### **Renderer**
OpenGL rendering pipeline featuring:
- Frustum culling with octree
- Transparency sorting (back-to-front)
- Blinn-Phong and Water (Gerstner waves) shaders
- Debug visualizations (AABBs, grid, Octree)

### **Scripting & Prefabs**
- **Lua Scripting:** Hot-reloadable scripts with exposed public variables.
- **Prefab System:** Reusable GameObject templates saved to JSON.
- **Script Editor:** Integrated IDE within the engine.

---

## 🎮 Controls

### Camera Navigation
| Action | Primary |
|--------|---------|
| Move Up/Down | `Space` / `Left Ctrl` |
| Zoom | `Mouse Wheel` |
| Free Look | `Right Mouse` + `WASD` |
| Orbit Object | `Left Alt` + `Left Mouse` |
| Focus | `F` |

### Editor Manipulation
| Mode | Key |
|------|-----|
| Translate / Rotate / Scale | `W` / `E` / `R` |
| Delete Object | `Backspace` |
| Save Script | `Ctrl + S` |

---

## 👥 Team
- **Pau Gutsens**
- **Edu García**
- **Haosheng Li**
- **Ana Alcaraz**

---

## 📜 License
**MIT License**
Copyright © 2025 Wave Engine Team
