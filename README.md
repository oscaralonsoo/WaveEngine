<h1 align="center">🌊 Wave Engine 🌊</h1>

<p align="center">
This project is a custom 3D game engine developed in C++ using OpenGL as the main graphics API.  
It integrates several external libraries such as Assimp (for 3D model loading), DevIL (for texture management), and ImGui (for the user interface).
</p>

<p align="center">
In version 2.0, we've evolved beyond simple FBX dependency, implementing a complete resource management system with custom file formats and significant rendering optimizations. The engine now features an advanced editor interface with scene serialization, resource management, and acceleration structures for optimal performance.
</p>

<p align="center">
🔗 <strong>GitHub Repository:</strong> <a href="https://github.com/Audra0000/Engine">https://github.com/Audra0000/Engine</a>
</p>

---

## 🎏 Team Members

- **Haosheng Li** — [GitHub: HaosLii](https://github.com/HaosLii)  
- **Ana Alcaraz** — [GitHub: Audra0000](https://github.com/Audra0000)

---

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
4. **Renderer:**  
   - Enable or disable **face culling** and choose its mode  
   - Toggle **wireframe mode**  
   - Change the **background color** of the scene
   - Toggle debug visualization for AABBs, octree,raycast, zBuffer  
5. **Hardware:**  
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
  - *GitHub documentation:* Opens the official documentation  
  - *Report a bug:* Opens `[Link to repo]/issues`  
  - *Download latest:* Opens `[Link to repo]/releases`  
  - *About:* Displays engine name, version, authors, libraries used, and MIT License  

---

## ✨ New Core Features 

### **Lua-Based Scripting System**

The engine now includes a built-in **Lua scripting system** that allows you to create, edit, and debug scripts directly inside the editor—no external IDE required.

---

### **Script Menu Options**

Include new script menu options: 
- **Script:** 
- Create Script: Create a new scripts on script folder with basic functions (Start() and Update()) 
- Show or hide Script editor 
- **File:** 
- Save scene now can also save scripts 
- Save scene will load the scene as usual but now with scripts
  
---

### **In-Editor Script Editing**

- Scripts can be **created, modified, and saved** directly inside the engine.
- Syntax errors are **reported in real time**, including the **line number** where the error occurs.
- **Variables declared at the top of the script** are automatically exposed in the **Inspector window**, where they can be edited.

---
### **Script Management**

Scripts can be attached to **GameObjects** and **Cameras**, allowing you to define custom behavior using Lua.

- **Gameobject:**
  - Find specific game objects, `tag` represents the name of another object. Use `"this"` to reference the GameObject that owns the script:
  ```lua
   GameObject = FindGameObject("tag")
   ``` 
  - Create game object using primitives ( `Cube`, `Pyramid`, `Sphere`, `Plane`, `Cylinder`): 
  ```lua
  GameObject = CreatePrimitive("Cube", "name")
  ```
  - Create game object using specific fbx:
  ```lua 
  GameObject = CreateGameObject("..//Assets//Tank.fbx", "name")
  ```
  - Delete game objects: 
  ```lua
  DeleteGameObject(GameObject)
  ```

- **Gameobject Modifiers:**
  - Transform Position: 
  ```lua
  SetPosition(GameObject, x, y, z)
  ```
  - Transform Rotation: 
  ```lua
  SetRotation(GameObject, x, y, z)
  ```
  - Transform Scale:
  ```lua 
  SetScale(GameObject, x, y, z)
  ```

- **Gameobject Data:**
  - Transform Position: 
  ```lua
  GetPosition(GameObject)
  ```
  - Transform Rotation: 
  ```lua 
  GetRotation(GameObject)
  ```
  - Transform Scale: 
  ```lua
  GetScale(GameObject)
  ```
- **Input Data:**
  - Mouse X Position: 
  ```lua
  Input.MouseX
  ```
  - Mouse Y Position: 
  ```lua
  Input.MouseY
  ```
  - Mouse left click: 
  ```lua
  Input.MouseLeft
  ```
  - Mouse right click:
  ```lua
   Input.MouseRight
   ```
  - Key state: 
  ```lua
  Input.A/Input.W/Input.S/...
  ```
---

<p align="center">
<sub>© 2025 Wave Engine — Developed by ....— MIT License</sub>
</p>
