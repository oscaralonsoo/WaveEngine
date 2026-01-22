<h1 align="center">🌊 Wave Engine 🌊 Audio System 🌊</h1>

<p align="center">
This project is a custom 3D game engine developed in C++ using OpenGL as the main graphics API.  
It integrates several external libraries such as Assimp (for 3D model loading), DevIL (for texture management), and ImGui (for the user interface).
This project was created as an assignement for the Video Game Engines class for Bachelor's degrees in Video Game Design and Development.
</p>

<p align="center">
In this version, a functioning audio system has been implemented through the intergration of Wwise. 
</p>

<p align="center">
🔗 <strong>GitHub Repository:</strong> <a href="https://github.com/bottzo/Motor2025/tree/Audio-System_VroomTeam"></a>
</p>
<p align="center">
🔗 <strong>Latest Release:</strong> link aqui
</p>

---

## 🐚 Audio System Team Members

- **Kai Caire** — [GitHub: KaiCaire](https://github.com/KaiCaire)
   - tasks stuff
- **Lara Guevara** — [GitHub: LaraGuevara](https://github.com/LaraGuevara)
   - Audio Listener and Audio Source components
   - Spatial Audio Implementation
   - Audio Components Serialization
   - Audio Components Inspector GUI
   - Move Component (for Spatial Audio test)
- **Marti Mach** — [GitHub: 0psycada](https://github.com/0psycada)
   - tasks stuff

## 🎏 Core Engine Team Members

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
- **Audio Source** (when selected):
   - Select Wwise Event from dropdown
   - Adjust Volume of the source
   - Play on Awake toggle
- **Audio Listener** (when selected):
   - Set as Default Listener
- **Move Component** (when selected):
   - Adjust Speed
   - Adjust Maximum Distance
   - Adjust movement Direction

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

## 🎶 Audio System Implementation 
- **Wwise Integration**
   - Wwise was intergrated into the Engine in order to implement the Audio System
- **Audio Components**
   - Audio Listener
      - Implemented Audio Listener component that can be added to Game Objects
      - Creating an Audio Listener will be set as the Main/Default Listener upon creation
         - Aditionally, an Audio Listener can be set as the Main/Default from the Inspector Window
      - Audio Listener Serialization
         - When a scene is saved, the Audio Listener component is saved
         - The Default Listener variable is saved
   - Audio Source
      - Implemented Audio Source component that can be added to Game Objects
      - Inspector settings for Audio Source
         - Can set Wwise Event from a drop down
            - On Program Launch, Wwise project's .json file is read to find all created Events
         - Volume can be changed
         - Set on Awake toggle
            - When Play mode is active, Event immediately plays
      - Audio Source Serialization
         - When a scene is saved, the Audio Source component is saved
         - The Wwise Event, Volume and Play on Awake variables are saved

giffff

- **Spatial Audio**
   - Both Audio Listener(s) and Audio Source(s) position are set for Wwise, allowing Spatial Audio adjusted to the Game Object positions
   - The Audio Component Game Objects can be visualized in the Wwise Game Object 3D Viewer
- **Music Mixing**
   - music mixing stuff here
- **Audio Effects**
   - audio effects stuff here

giffff

- **Additional Implementations**
   - Move Component
      - To test the Spatial Audio intergration, a simple Move Component was created to move a Game Object along one direction back and forth
      - Can be customized from the Inspector 
   - Add Component in Inspector
      - Added a button in the Inspector when a Game Object is selected
      - Can add:
         - Audio Source
         - Audio Listener
         - Move Component

## Scene Save and Load
el video aqui
---

<p align="center">
<sub>© 2025 Wave Engine  — MIT License</sub>
</p>
