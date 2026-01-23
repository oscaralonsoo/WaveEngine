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
🔗 <strong>GitHub Repository:</strong> <a href="https://github.com/bottzo/Motor2025/tree/Audio-System_VroomTeam">https://github.com/bottzo/Motor2025/tree/Audio-System_VroomTeam</a>
</p>

<p align="center">
🔗 <strong>Latest Release:</strong> link aqui
</p>

---

## 🐚 Audio System Team Members

- **Kai Caire** — [GitHub: KaiCaire](https://github.com/KaiCaire)
    <img src="https://github.com/user-attachments/assets/c232d27c-41bc-4696-a234-1969de811b18" width="50%">
   - Wwise Low Level Integration
   - Wwise Events & Buses Setup
   - Interactive Music Implementation 
   - Reverb Effect Configuration
   - Reverb Zone Preset GUI implementation
   
- **Lara Guevara** — [GitHub: LaraGuevara](https://github.com/LaraGuevara)
  <img src="https://github.com/user-attachments/assets/4b5283bd-056e-48a9-80cd-aed7f3da277a" width="50%">
   - Audio Listener and Audio Source components
   - Spatial Audio Implementation
   - Audio Components (Source & Listener) Serialization
   - Audio Components (Source & Listener) Inspector GUI
   - Move Component (for Spatial Audio test)

- **Marti Mach** — [GitHub: 0psycada](https://github.com/0psycada)
  <img src="https://github.com/user-attachments/assets/840841b0-04e5-434e-8dfa-ff3919d2c0aa" width="50%">
   - Audio Effect Implementation
   - Reverb Zone component
   - Reverb Zone Serialization
   - Reverb Zone component Inspector GUI

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
   - Select Wwise Event from drop-down
   - Adjust Volume of the source
   - Play on Awake toggle
- **Audio Listener** (when selected):
   - Set as Default Listener
- **Reverb Zone** (when selected):
   - Change Zone Shape from drop-down selection
   - Change the Reverb Preset from drop-down selection
   - Set the Zone's...
      - If Zone Shape is Box :arrow_right: Half Extents
      - If Zone Shape is Sphere :arrow_right: Radius
   - Set the Aux Bus Name
   - Change the Wet Level
   - Set Priority
   - Enable/Disable Toggle
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
   - Audio Source
      - Implemented Audio Source component that can be added to Game Objects
      - Inspector settings for Audio Source:
         - Can set Wwise Project's Event from a drop down
         - Volume can be changed
         - Set on Awake toggle
            - When Play mode is active, Event immediately plays
          
![EnignesAudio_SourceCreation](https://github.com/bottzo/Motor2025/blob/Audio-System_VroomTeam/readmeFiles/createSource.gif)

- **Spatial Audio**
   - Both Audio Listener(s) and Audio Source(s) position are set for Wwise, allowing Spatial Audio adjusted to the Game Object positions
- **Music Mixing**
   - Audio Sources can be set to play Music Playlist Events, allowing music mixing and fades
- **Audio Effects**
   - A Reverb Zone component for Game Objects has been implemented, allowing to apply a Reverb Effect inside a set Zone in the Scene
   - Reverb Zones interact with the Main Audio Listener to apply the audio effect
   - It is important to note that audio effects are not applied to background music elements
   - Inspector Settings for Reverb Zone:
      - Change Reverb Zone Shape (Box and Sphere options)
         - Depending on the selected shape radius (for Sphere) or half extents (for Box) can be changed
      - Reverb Preset drop-down
      - See current Aux Bus Name
      - Change Wet Level
      - Set Priority
      - Toggle to set as Enable/Disabled 

![EnignesAudio_ReverbCreation](https://github.com/bottzo/Motor2025/blob/Audio-System_VroomTeam/readmeFiles/createReverb.gif)

- **Audio Component Serialization**
   - When a scene is saved, Audio Source and Audio Listener components are saved
      - For Audio Listeners :arrow_right: The Default Listener variable is saved
      - For Audio Sources :arrow_right: The Wwise Event, Volume and Play on Awake variables are saved
      - For Reverb Zone :arrow_right: Shape, Radius, Extents, Aux Bus Name, Wet Level, Priority and Enabled variables are saved
- **Additional Implementations**
   - Move Component
      - To test the Spatial Audio intergration, a simple Move Component was created to move a Game Object along one direction back and forth
      - Can be customized from the Inspector
      - The component variables are also serialized when a scene is saved
   - "Add Component" in Inspector
      - Added a button in the Inspector when a Game Object is selected
      - Can add:
         - Audio Source
         - Audio Listener
         - Reverb Zone
         - Move Component
   - Configurable and serializable Roomverb Presets from Wwise by a drop down in the Inspector in the Reverb Zone component, both in editor and at runtime.
     Managed in Wwise through Individual Auxiliary Busses
       - Cathedral: A massive stone interior with long, shimmering decay and grand scale.
       - Inside My Head: A tight, claustrophobic resonance that mimics internal dialogue or extreme proximity.
       - Aluminum Tank: A high-frequency, ringing resonance typical of hollow metal containers or industrial pipes.
       - Absorption: A deadened, muffled space that swallows high frequencies, perfect for padded rooms or thick carpets
       - Large Plate: A classic, dense studio reverb that provides a smooth, metallic wash to any sound.
       - Long Dark Hall: An eerie, expansive tunnel effect with a cold atmosphere and distant echoes.
       - Robotic: A glitched, digitized reverb with metallic artifacts that sounds like an old, slightly malfunctioning robot
       - Outside: A subtle, open-air ambience with natural reflections and zero "boxy" build-up.

## Scene Creation
![EnginesAudio_SaveLoad](https://github.com/bottzo/Motor2025/blob/Audio-System_VroomTeam/readmeFiles/createScene_video.mp4)

<p align="center">
<sub>© 2025 Wave Engine  — MIT License</sub>
</p>
