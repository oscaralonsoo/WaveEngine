<h1 align="center">🌊 Wave Engine 🌊 Audio System 🌊</h1>

<p align="center">
This project is a custom 3D game engine developed in C++ using OpenGL as the main graphics API.  
This project was created as part of the Game Engines course at CITM (Centre de la Imatge i la Tecnologia Multimèdia).
</p>

<p align="center">
The engine integrates several external libraries including Assimp (for 3D model loading), DevIL (for texture management), ImGui (for the user interface), and Lua (for scripting). Also has a resource management system with custom file formats, an advanced editor interface, scene serialization, and a Lua scripting system that allows runtime gameplay programming.
</p>

<p align="center">
🔗 <strong>GitHub Repository:</strong> <a href="https://github.com/bottzo/Motor2025/tree/Scripting-HaoshengAna2">https://github.com/bottzo/Motor2025/tree/Scripting-HaoshengAna2</a>


<p align="center">
📦 <strong>Latest Release:</strong> <a href="https://github.com/bottzo/Motor2025/releases/tag/Scripting2">https://github.com/bottzo/Motor2025/releases/tag/Scripting2</a>
</p>

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
The core module that manages the engine's lifecycle. Contains all modules and controls the game loop (Awake → Start → Update → PostUpdate → CleanUp). Also handles Play/Pause/Stop states.

### **ModuleScene**
Manages the scene and GameObject tree. Has a root node from which all objects hang. Also maintains an Octree for spatial optimization (frustum culling, raycasting).

### **GameObject / Components**
Entity-component system:
- **GameObject:** Container with name, transform, children, and components
- **Transform:** Position, rotation, scale (local and global)
- **ComponentMesh:** 3D geometry
- **ComponentMaterial:** Textures and materials
- **ComponentCamera:** Game camera
- **ComponentScript:** Lua scripts
- **ComponentAudioSource**: Audio Sources (Wwise sound events)
- **ComponentAudioListener**: Audio Listener 
- **ComponentReverbZone**: Areas with audio effects

### **ModuleResources**
Resource system with unique UIDs. Manages loading/unloading of:
- **ResourceMesh** - 3D meshes
- **ResourceTexture** - Textures
- **ResourceScript** - Lua scripts
- **ResourcePrefab** - Prefabs

### **Renderer**
Draws the scene using OpenGL. Manages:
- Frustum culling with octree
- Mesh rendering with materials
- Debug drawing (AABBs, grid, Frustum culling)

### **ModuleEditor**
Editor system with ImGui windows:
- **HierarchyWindow** - Object tree
- **InspectorWindow** - Selected object properties
- **AssetsWindow** - File browser
- **ScriptEditorWindow** - Lua script editor
- **GameWindow / SceneWindow** - Viewports

### **SelectionManager**
Manages object selection in the editor (single/multi-select).

### **Resource Management System**
- Complete asset pipeline with automatic conversion to custom file formats
- Assets stored in a structured "Assets" folder, with optimized versions cached in "Library"
- Reference counting ensures resources are loaded only once regardless of usage count
- Automatic regeneration of Library folder from Assets and metadata files
- Support for importing new assets at runtime

### **Performance Optimizations**
- **Frustum Culling:** Objects outside the camera view are not rendered
- **Octree Spatial Partitioning:** Accelerates both rendering culling and object selection
- **Debug Visualizations:** Toggle visual representations of AABBs, octree nodes, and frustum

### **Scene Management**
- Scene serialization to custom file format
- Automatic loading of default scene ("StreetEnvironment")
- Complete GameObject hierarchy support with parent-child relationships
- Runtime transformation of objects (position, rotation, scale)

### **Camera System**
- Configurable camera component with adjustable parameters
- Selection system using raycasting with octree
- Visual feedback for selection operations

### **Custom File Formats**
- Proprietary formats for models, textures and scenes
- Metadata files storing import settings and dependencies

### **Additional Features**
- **Transparent textures**
- **zBuffer**
- **Assets icons**
- **Asset Deletion:** Delete assets directly from the explorer with automatic cleanup of associated Library files
- **Import Settings:** Basic implementation of import options for different asset types:
  - **Textures:** Control filtering modes, max texture size, and flip options (X/Y axes)
  - **Meshes:** Configure global scaling, axis configuration, Post-processing options: generate normals, flip uv, optimize meshes
  - **Metadata:** All import settings are saved in .meta files to ensure proper regeneration of the Library folder

---

## 🎮 Scripting System (High-Level Feature)

<p align="center">
  <img src="GifEditor.gif" alt="Editor Scripting Demo" width="500"> <img src="Gif_Juego.gif" alt="Gameplay Scripting Demo" width="500">
</p>

The engine's scripting system allows users to program custom behaviors for GameObjects using the Lua language. The architecture is divided into three main components: the ScriptManager that maintains the global Lua state, the ComponentScript that connects scripts with GameObjects, and the ResourceScript that manages script files as engine resources.

### **ScriptManager**
The ScriptManager is the core module of the scripting system. During engine initialization, this module creates a Lua state and loads the language's standard libraries. Additionally, it registers all engine APIs that will be available to scripts.

The exposed APIs include utility functions such as logging (Engine.Log), input functions (Input.GetKey, Input.GetKeyDown, Input.GetMousePosition), and Time.GetDeltaTime to obtain the elapsed time between frames. It also exposes an API to manipulate GameObjects: search for objects by name (GameObject.Find), create them (GameObject.Create), destroy them (GameObject.Destroy), and access the Transform to modify position, rotation, and scale.

An important feature is the deferred operations system. When a script requests an operation that modifies the scene, it is queued and executed during the engine's PostUpdate phase, avoiding concurrency issues.

### **ComponentScript**
The ComponentScript is attached to a GameObject to give it behavior through a Lua script. Each instance maintains its own isolated environment within the global Lua state, allowing multiple objects to use the same script without interfering with each other.

The lifecycle is simple: when the engine enters Play mode, the script's Start function is called once. Then, on each frame, the Update function is called passing the self object and delta time as parameters.

### **Public Variables**
Scripts can declare a table called public to expose variables editable from the editor's Inspector. These variables can be numbers, strings, booleans, or vectors. Values are serialized with the scene and preserved during hot reload.

### **ResourceScript**
The ResourceScript represents a Lua file as a resource managed by the engine. It stores the timestamp of the file's last modification to detect changes and enable automatic hot reload of scripts while preserving public variable values.

### **Metatables and Userdata**
To expose C++ objects to Lua, the system uses metatables and userdata. Transforms and GameObjects are represented as userdata with metatables that implement the __index metamethod, intercepting access to properties and methods. Operations that modify the scene are queued for deferred execution during PostUpdate.

The system includes validations to prevent crashes, verifying that pointers are valid before accessing objects. Lua errors are caught using pcall and reported without stopping engine execution.

### **Script Editor**
The Script Editor is an integrated window in the engine that allows editing Lua files directly without needing an external editor. It inherits from EditorWindow and uses ImGui to provide essential functionalities like syntax highlighting, line numbering, real-time error detection, and a tab system for working with multiple files simultaneously. It operates in two modes: preview mode (with full syntax highlighting but no direct editing) and edit mode (allows text modification but disables highlighting). Users can toggle between modes with a double-click or Control+H.

The editor includes a complete file management system with draggable tabs, saving via Control+S and a search and replace with Control+F. It allow files to be opened from the Assets window with a double-click, and saved changes automatically trigger the hot reload system to reload scripts in use. The error detection displaying issues in a bottom panel and coloring error lines in red. To optimize performance with large files, it only renders visible lines on screen.

---

## 🎁 Prefab System (Extra Feature)

The prefab system allows saving complete GameObjects as reusable templates that can be instantiated multiple times during game execution. A prefab captures an object's hierarchical structure, including all its components, properties, and child objects, storing it in a JSON file that can be loaded later to create identical copies.

### **System Architecture**
The prefab system consists of three main classes working together. The Prefab class represents an individual template and contains the serialized data of the original GameObject. The PrefabManager acts as a central registry that stores and manages all loaded prefabs in memory. Finally, the ResourcePrefab integrates prefabs with the engine's resource system, allowing them to be managed with unique UIDs just like textures or meshes.

### **The Prefab Class**
The Prefab class encapsulates a serialized GameObject's data and provides methods to save, load, and instantiate objects. Internally, it stores a JSON object containing all necessary information to reconstruct the original GameObject.

When creating a prefab from an existing GameObject, the SaveFromGameObject method traverses the object and all its components, serializing each one in JSON format. This includes the object's name, its Transform with position, rotation and scale, and all additional components such as meshes, materials, or scripts. If the GameObject has children, these are also serialized, preserving the complete hierarchy.

The resulting file is saved to disk with a .prefab extension and contains a readable JSON structure that can be inspected or manually edited if necessary.

### **PrefabManager**
The PrefabManager is a singleton that maintains a dictionary of all loaded prefabs, indexed by name. It provides a centralized interface to load prefabs from files, create new prefabs from existing GameObjects, and instantiate copies of already loaded prefabs.

The InstantiatePrefab method searches for the prefab by name in the dictionary and delegates creation to the Prefab class's Instantiate method. If the prefab isn't loaded, it returns a null pointer and logs an error to the console.

### **ResourcePrefab**
The ResourcePrefab integrates prefabs with the engine's resource system. Each .prefab file in the Assets folder is automatically registered as a resource with its own unique UID. This allows referencing prefabs consistently even if files are renamed or moved.

This integration allows Lua scripts to reference prefabs by filename, while internally the system resolves the reference to the correct UID by searching among all prefab-type resources.

### **Instantiation Process**
Prefab instantiation is a process that completely reconstructs a GameObject from its serialized data. First, a new GameObject is created with the name stored in the prefab. Then it iterates over the list of serialized components, creating each one and calling its Deserialize method to restore its properties.

The Transform is restored with the original position, rotation, and scale. Mesh and material components load their associated resources using the stored UIDs. ComponentScripts load their scripts and restore public variable values that were saved in the prefab.

If the prefab contains child objects, these are instantiated recursively following the same process, and parent-child relationships are established to reconstruct the original hierarchy.

### **Integration with the Scripting System**
Lua scripts can instantiate prefabs using the Prefab.Instantiate function, which receives the prefab filename as a parameter. This operation executes in a deferred manner to maintain consistency with other scene-modifying operations.

A common pattern is using global Lua variables to pass initialization information to newly created prefabs. For example, when a turret fires a bullet, before instantiating the bullet prefab it stores the spawn position, movement direction, and other parameters in a global variable. The bullet's script reads this data during its first Update to configure itself correctly.

### **Component Serialization**
Each component type implements its own Serialize and Deserialize methods that define how its properties are saved and restored. The Transform serializes three vectors for position, rotation, and scale. ComponentMesh saves the UID of its assigned mesh. ComponentMaterial stores the texture UID and material properties such as colors or shader parameters.

ComponentScript serializes the associated Lua script's UID along with all public variables and their current values. This allows each prefab instance to have different values in its public variables, which are preserved when saving and loading the scene or creating the prefab.

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
