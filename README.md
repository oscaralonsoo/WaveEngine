# 🌊 Wave Engine
<h1 align="center">🌊 Wave Engine 🌊 Audio System 🌊</h1>

> **A Custom 3D Game Engine built with C++ and OpenGL**

Wave Engine is a feature-rich game engine developed as part of the Game Engines course at CITM-UPC. This project integrates several external libraries including Assimp (model loading), DevIL (textures), ImGui (UI), Lua (scripting), and NVIDIA PhysX (physics).

The engine focuses on modularity and high-performance rendering, featuring a complete shader pipeline (Blinn-Phong, Gerstner waves), a robust resource management system, and a Lua scripting system for runtime behavior.
<p align="center">
🔗 <strong>GitHub Repository:</strong> <a href="https://github.com/bottzo/Motor2025/tree/Scripting-HaoshengAna2">https://github.com/bottzo/Motor2025/tree/Scripting-HaoshengAna2</a>


**Technologies:** C++17 | OpenGL 4.6 | GLSL 330 | ImGui | Assimp | DevIL | Lua | PhysX | tracy

---

## 📹 Video Demonstration

[![Ver video](https://img.youtube.com/vi/79wXfoxlBjI/maxresdefault.jpg)](https://www.youtube.com/watch?v=79wXfoxlBjI)

<p align="center"><em>Click the image above to watch the full engine demonstration video</em></p>

---

## 🎏 Core Engine Team Members

- **Haosheng Li** — [GitHub: HaosLii](https://github.com/HaosLii)
- **Ana Alcaraz** — [GitHub: Audra0000](https://github.com/Audra0000)

The implementation of the Lua-based scripting system was carried out jointly. Both team members collaborated on integrating the base architecture, although Ana Alcaraz took on a slightly larger implementation load for the foundation. We divided the specific functionalities as follows:

**Ana Alcaraz**: Handled the management of object lifecycle and integration with the editor (corresponding to 30% and 20% of the task respectively). This included:

- Implementing runtime prefab creation (bullets). 
- Managing timers for entity destruction. 
- Publishing environment variables in the inspector. 
- Integrating a text editor window within the engine to modify scripts in real-time. 

**Haosheng Li** handled the movement logic (50%). This includes:

- Developing scripts for tank movement and rotation using keyboard input. 
- Implementing the script for the turret to rotate and aim toward the mouse position in the scene. 

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
- **ComponentParticleSystem:** Particle effects
- **Physics Components:** Rigidbody and Various Colliders (Box, Sphere, Capsule, Mesh)
- **ComponentMaterial:** Textures and materials
- **ComponentCamera:** Game camera
- **ComponentScript:** Lua scripts
- **ComponentAudioSource**: Audio Sources (Wwise sound events)
- **ComponentAudioListener**: Audio Listener 
- **ComponentReverbZone**: Areas with audio effects

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

## 🎶 Audio High-Level System 

## Wwise Integration
The Audio System has been implemented through the Audiokinetic Wwise game audio middleware, which generates specialized audio resources called "soundbanks".

It also provides a profiler tool to debug the engine audio events in real time.

## Audio Components
### **Audio Listener**
Can be added to Game Objects in the Inspector, and will set the Game Object it has been attached to as the Main/Default Listener upon creation. 

Additionally, an Audio Listener can be set as the Main/Default from the Inspector Window, and is serialized when saving the scene.

### **Audio Source**

Can be added to Game Objects in the Inspector, and in the component window, the Wwise event can be selected from a dropdown containing all the existing events. 
The source's volume can be changed with a slider and it can be set to Play on Awake with a checkbox, so that the event immediately plays when Play Mode is active.
All the properties mentioned are serialized/deserialized in the scene file.
      
![EnignesAudio_SourceCreation](https://github.com/user-attachments/assets/9688b6dd-f6be-4406-a2a2-fd2dd6b5ecd1)

### **Reverb Zone**
Creates a trigger area in which Roomverb Presets from Wwise are applied, distorting the sound while inside the zone. All its properties are serialized.

It can be enabled/disabled via a checkbox toggle, its shape can be changed into a Sphere or Box, and there's sliders for modifying wet level and priority.

The different effects are managed in Wwise through Individual Auxiliary Busses. The available presets can be selected via a dropdown in the Inspector, and include:
    - Cathedral: A massive stone interior with long, shimmering decay and grand scale.
    - Inside My Head: A tight, claustrophobic resonance that mimics internal dialogue or extreme proximity.
    - Aluminum Tank: A high-frequency, ringing resonance typical of hollow metal containers or industrial pipes.
    - Absorption: A deadened, muffled space that swallows high frequencies, perfect for padded rooms or thick carpets
    - Large Plate: A classic, dense studio reverb that provides a smooth, metallic wash to any sound.
    - Long Dark Hall: An eerie, expansive tunnel effect with a cold atmosphere and distant echoes.
    - Robotic: A glitched, digitized reverb with metallic artifacts that sounds like an old, slightly malfunctioning robot
    - Outside: A subtle, open-air ambience with natural reflections and zero "boxy" build-up.


## Audio Features

### Spatial Audio
Both Audio Listener(s) and Audio Source(s) position are set for Wwise, allowing Spatial Audio adjusted and attenuated according to the Game Object's positions.

### Interactive Music
Wwise Events support music track mixing, layering and fading transitions, allowing for AudioSources to play interactive music.


![EnignesAudio_ReverbCreation](https://github.com/user-attachments/assets/d0db756a-5ea7-4618-a65e-3ed5d6c27a9c)

https://github.com/user-attachments/assets/da7769be-5b25-400d-b88d-4912b46e189f


## 🦀 Controls

| Action | Key 1 | Key 2 |
|------------|------------|------------|
| Up | Space | |
| Down | Left Ctrl | |
| Zoom | Mouse wheel | |
| Velocity ×2 | Hold Shift | |
| Free movement | Right Mouse Button | WASD |
| Orbit | Left Alt | Left Mouse Button |
| Focus | F | |
| Select | Left Mouse Button | |
| Multiple select | Shift | Left Mouse Button |
| Delete object | Backspace | |

| Gizmo Action | Key 1 | Key 2 |
|------------|------------|------------|
| Move | W | |
| Rotate | E | |
| Scale | R | |
| Toggle Coordinate System | T | |

| Script Editor Action | Key 1 | Key 2 |
|------------|------------|------------|
| Save | Ctrl + S | |
| Save all | Ctrl + Shift + S | |
| Close Tab | Ctrl + W | |
| Find & Replace | Ctrl + F | |
| Highlighting | Ctrl + H | |

---

## 🐠 User Interface

### **Console**
The console logs all engine events and processes, such as:
- Loading geometry (via **ASSIMP**)
- Loading textures (via **DevIL**)
- Resource management operations
- Initialization of external libraries
- Application flow and error messages

Additionally, it includes several **interactive options**:
- **Clear:** Erases all current console messages  
- **Log filters:** Enable or disable the display of specific types of logs (info, warnings, errors)

---

### **Configuration**
This window is divided into **five tabs**:

1. **FPS:** Displays the current frame rate and performance data.  
2. **Window:** Allows full customization of the application window:  
   - Adjust size and resolution  
   - Toggle **fullscreen** or **borderless** mode  
   - Enable/disable **resizable** window  
3. **Camera:**  
   - Adjust camera settings 
   - Reset camera settings  
   - View current **camera position**
   - Displays a summary of **camera controls**
   - Change current active camera
   - Displays current active camera 
4. **Audio**
   - Adjust General Music Volume
   - Adjust General SFX Volume
5. **Renderer:**  
   - Enable or disable **face culling** and choose its mode  
   - Toggle **wireframe mode**  
   - Change the **background color** of the scene
   - Toggle debug visualization for AABBs, octree, raycast, zBuffer  
6. **Hardware:**  
   - Displays detailed information about the system hardware in use  

---

### **Assets Window**
A dedicated panel to manage all project resources:
- Browse assets organized
- Import new assets via drag-and-drop
- Delete assets (automatically removes associated files in Library folder)
- Visualize reference
- View asset and import settings

---

### **Hierarchy**
Displays all GameObjects in the current scene, allowing:
- Selection of scene objects
- Reparenting objects (drag to change hierarchy)
- Renaming (double click)

---

### **Inspector**
Provides detailed information and transformation options for the selected GameObject:
- **Gameobject:**
   - Set active camera (only camera)
   - Reparenting objects (list)
   - Creating empty GameObjects
   - Deleting objects
- **Gizmo:**
   - Change gizmo mode
   - World/Local 
- **Transform:** Modify **position**, **rotation**, and **scale** directly.  
  Includes a **reset option** to restore default values.  
- **Mesh:**  
  - Displays mesh data and allows **normal visualization** (per-triangle / per-face)  
  - Select any imported mesh from the Assets window   
- **Material:**  
  - Shows texture path and dimensions  
  - Preview textures with optional checker pattern  
  - Select textures from the Assets window  
- **Camera Component** (when selected):  
  - Active frustum culling
  - Toggle debug visualization for frustum culling
- **Script Component** (when added):  
  - Change script
  - Remove script
  - Reload script
  - Public variables editing
- **Audio Source Component** (when selected):
   - Select Wwise Event from drop-down
   - Adjust Volume of the source
   - Play on Awake toggle
- **Audio Listener Component** (when selected):
   - Set as Default Listener
- **Reverb Zone Component** (when selected):
   - Change Zone Shape from drop-down selection
   - Change the Reverb Preset from drop-down selection
   - Set the Zone's...
      - If Zone Shape is Box :arrow_right: Half Extents
      - If Zone Shape is Sphere :arrow_right: Radius
   - Set the Aux Bus Name
   - Change the Wet Level
   - Set Priority
   - Enable/Disable Toggle


---

### **Toolbar**

Includes the following menu options:

- **File:**
  - Save scene
  - Load scene 
  - Exit the program  
- **View:** Show or hide any of the editor windows
  - Layout (Save, load, auto save)  
- **Cameras:**  
  - Create camera
- **Gameobjects:**
  - Create primitves
  - Add rotate component
- **Help:**
  - *GitHub web:* Opens the repository web
  - *GitHub documentation:* Opens the official documentation  
  - *Report a bug:* Opens `[Link to repo]/issues`  
  - *Download latest:* Opens `[Link to repo]/releases`  
  - *About:* Displays engine name, version, authors, libraries used, and MIT License  

---

<p align="center">
<sub>© 2025 Wave Engine  — MIT License</sub>
</p>
