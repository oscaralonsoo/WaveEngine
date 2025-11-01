<h1 align="center">🌊 Wave Engine 🌊</h1>

<p align="center">
This project is a custom 3D game engine developed in C++ using OpenGL as the main graphics API.  
It integrates several external libraries such as Assimp (for 3D model loading), DevIL (for texture management), and ImGui (for the user interface).
</p>

<p align="center">
Currently, the engine is in its first version, which focuses on building a solid foundation.  
It supports loading FBX models with textures, transforming them in position, rotation, and scale, and provides configuration options to customize the engine’s behavior.  
This stage represents the core structure upon which future features and systems will be developed.
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

---

## 🐠 User Interface

### **Console**
The console logs all engine events and processes, such as:
- Loading geometry (via **ASSIMP**)
- Loading textures (via **DevIL**)
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
   - Adjust different **camera sensitivities**  
   - Reset camera settings  
   - View current **camera position**  
   - Displays a summary of **camera controls**  
4. **Renderer:**  
   - Enable or disable **face culling** and choose its mode  
   - Toggle **wireframe mode**  
   - Change the **background color** of the scene  
5. **Hardware:**  
   - Displays detailed information about the system hardware in use  

---

### **Hierarchy**
Displays all loaded GameObjects in the current scene, allowing easy selection and organization of scene elements.

---

### **Inspector**
Provides detailed information and transformation options for the selected GameObject:
- **Transform:** Modify **position**, **rotation**, and **scale** directly.  
  Includes a **reset option** to restore default values.  
- **Mesh:** Displays mesh data and allows **normal visualization** (per-triangle / per-face).  
- **Material:** Shows texture path and dimensions, with an optional **checker pattern preview**.

---

###  Toolbar

Includes the following menu options:

- **File:** Exit the program  
- **View:** Show or hide any of the editor windows  
- **Help:**
  - *GitHub documentation:* Opens the official documentation  
  - *Report a bug:* Opens `[Link to repo]/issues`  
  - *Download latest:* Opens `[Link to repo]/releases`  
  - *About:* Displays engine name, version, authors, libraries used, and MIT License  

---

## ✨ Extra Features

- **Object Selection:**  
  Select scene objects directly using the mouse.  
- **Transformation Tools:**  
  Move, rotate, and scale objects in the scene using interactive gizmos.  
- **Customisation Options:**  
  Multiple configuration settings allow you to tailor the engine’s visuals and performance to your needs.

---

<p align="center">
<sub>© 2025 Wave Engine — Developed by Haosheng Li & Ana Alcaraz — MIT License</sub>
</p>

