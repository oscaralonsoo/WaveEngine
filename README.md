<h1 align="center">🌊 Wave Engine | Particle System RGSEngine 🌊</h1>

<p align="center">
This project is a custom 3D game engine developed in C++ using OpenGL as the main graphics API. Developed as part of the <strong>Motors de Videojocs</strong> course at CITM.
</p>

<p align="center">
In this latest release (Assignment 3), we have implemented a high-level <strong>Particle System</strong>. This advanced, modular, CPU-based simulation system is fully integrated into the engine's <strong>Entity-Component-System (ECS)</strong> architecture, designed to create complex visual effects such as fire, smoke, and explosions through a completely data-driven workflow.
</p>

<p align="center">
🔗 <strong>GitHub Repository:</strong> <a href="https://github.com/bottzo/Motor2025/tree/Particles_RGSEngine">https://github.com/bottzo/Motor2025/tree/Particles_RGSEngine</a><br>
💾 <strong>Latest Release:</strong> <a href="link_a_nuestra_release">https://github.com/bottzo/Motor2025/releases</a>
</p>

---

## 🎏 Team Members

- **Pablo Sanjose** — [GitHub: XXPabloS](https://github.com/XXPabloS)
- **Víctor González** — [GitHub: TheWolfG145](https://github.com/TheWolfG145)
- **Claudia Ruiz** — [GitHub: Claurm12](https://github.com/Claurm12)

---

## 🔍 How To Use

<p align="center">The Particle System is fully integrated into the engine's Entity-Component-System (ECS) architecture, functioning exactly like a standard component (similar to Unity). This means any GameObject in your scene can become a particle emitter.</p>

### **Adding a Particle System**
1. Select a GameObject from the **Hierarchy** window (or create an empty one).
2. In the **Inspector** window, click the **"Add Component"** button.
3. Select **"Particle System"** from the dropdown menu.

> **Note:** The particle emitter automatically links to the GameObject's Transform component. If you move, rotate, or animate the GameObject, the particle source will follow it automatically.

### **Inspector & Editor Controls**
The component exposes a complete UI for real-time editing:
- **Resource Management:** Drag-and-drop texture assignment from the Assets panel.
- **Simulation Control:** Buttons to *Play*, *Pause*, or *Stop* simulation directly in the editor.
- **Burst System:** Manual trigger for instant particle generation, useful for debugging explosions.
- **Preset Management:**
  - **Save:** Serializes the current configuration to a JSON `.particle` file.
  - **Load:** Hot-reloads configuration from disk without recompiling.

### **functionality Showcase**

| **Editor Workflow** | **In-Game Simulation** |
|:-------------------:|:----------------------:|
| ![Humo](https://github.com/user-attachments/assets/8f9e94eb-8920-4417-b690-22ae1a76120b) |![Fireworks](https://github.com/user-attachments/assets/f56da579-f60f-4065-9a80-a1ed7b64e665) |
| *Smoke effect in chimneys* | *Fireworks effect* |

### **Creation Process**

<p align="center">
  <a href="https://www.youtube.com/watch?v=esKwzRaQlhk">
    <img src="https://img.youtube.com/vi/esKwzRaQlhk/maxresdefault.jpg" alt="Creation Process" width="600"/>
  </a>
  <br>
  <em>(Click the image to watch a One-minute time-lapse video showing the process of using the particle system)</em>
</p>

### **Creation Process Smoke effect in chimneys**

To showcase the versatility and expressiveness of the engine's Particle System, a persistent ambient smoke simulation has been implemented for architectural elements. This feature demonstrates continuous particle emission, fine-tuned interpolation, and scene integration.

#### Technical Implementation
The chimney smoke effect is managed by `ModuleScene` and consists of a persistent, multi-emitter setup designed for atmospheric realism:

##### 1. Emitter Placement
* Four smoke emitters are instantiated at hardcoded chimney positions during scene initialization via `CreateChimneySmoke()`.
* Each emitter is a distinct GameObject (`Chimney_Smoke_1` through `Chimney_Smoke_4`) positioned at realistic architectural locations within the scene.
* Transform components position the emitters at precise 3D coordinates to align with chimney structures.

##### 2. Particle Configuration
The `ConfigureSmokeEffect()` method applies specialized settings to each `ComponentParticleSystem`:

* **Emission Settings:** Cone-shaped emitter with a narrow angle (**1.0°**) and small radius (**0.1**) to simulate natural chimney draft.
* **Lifecycle:** Particles persist for **1.0–2.2 seconds** with a controlled emission rate of **10 particles/second**, maintaining **60 particles maximum** per emitter.
* **Visual Properties:**
    * Color interpolation from semi-transparent gray (**0.45, 0.45, 0.48, 0.5**) to lighter, fully transparent (**0.7, 0.7, 0.72, 0.0**) simulating natural smoke dissipation.
    * Size grows from **0.05** to **0.2** units, mimicking realistic smoke expansion.
    * Random rotation speeds (**-10** to **10 deg/s**) add organic movement.
* **Physics Simulation:** Custom gravity vector (**0.03, 0.2, 0.04**) creates upward drift with slight horizontal dispersion, replicating wind influence.

##### 3. Rendering
* Standard alpha blending (not additive) ensures smoke appears translucent and layered, matching real-world atmospheric scattering.
* Particles are sorted by distance to camera for correct transparency rendering.

*This implementation demonstrates how the modular Particle System can create subtle, long-running environmental effects that enhance scene believability without manual animation or excessive performance overhead.*

### **Creation Process Fireworks effect**

To demonstrate the capabilities of the engine's modular **Particle System**, a dynamic firework simulation has been implemented. This feature showcases real-time object instantiation, physics-based particles, and automatic memory management.

**Technical Implementation:**
The lifecycle of a firework is managed by `ModuleScene` and consists of two distinct phases, ensuring a realistic effect without performance drops:

1.  **Ascent Phase:**
    * Instantiates a rocket with a **Cone Emitter** attached to its base to simulate a smoke trail.
    * The particle module is configured with **local simulation** to follow the rocket's position perfectly as it ascends.
    * Uses a random timer (fuse) and random starting position to ensure variety.

2.  **Detonation Phase:**
    * Once the fuse timer expires, the rocket is destroyed, and a separate "Explosion" object is spawned at its exact last position.
    * This object uses a **Sphere Emitter** with `Burst()` functionality to emit 150+ particles instantly in all directions.
    * **Randomization:** The explosion color is procedurally generated at runtime.

3.  **Cleaning:**
    * To prevent memory leaks, both the rocket (after exploding) and the explosion effect (after fading out) are automatically flagged with `MarkForDeletion()`.
    * The engine cleans up these "dead" GameObjects at the end of the frame, keeping the hierarchy clean and performance stable.


---

## 🦀 Controls

| Action | Key | Description |
|------------|------------|------------|
| **Spawn Firework Demo** | 1 | Launches a rocket trail that explodes into a randomized firework effect. |

---


## 🔧 Technical Details

### **Rendering & Physics**
- **Billboarding:** Quad rendering that aligns perfectly with the active 
camera.
- **Z-Sorting:** Real-time depth sorting to handle alpha transparency artifacts correctly.
- **Additive Blending:** Optional rendering mode for light-emitting effects (fire, sparks).
- **Physics Integration:** Support for gravity vectors, velocity randomization, and angular velocity (spin).

### **Emitter Architecture**
The system supports multiple shape emissions:
- **Box:** Random emission within a volume.
- **Sphere:** Omnidirectional emission (explosions).
- **Cone:** Directional emission with angle control (flamethrowers, fountains, chimneys).

### **Lifecycle Interpolation**
Smooth transition of properties over time:
- **Color:** Fade in/out and color shifting (gradients).
- **Size:** Growth or shrinkage over particle life.

### **Animation System**
- **Sprite Sheet Support:** Ability to divide a texture into Rows × Cols.
- **UV Animation:** Updates texture coordinates over the particle's lifetime to play frame-by-frame animations (e.g., evolving smoke).

---

<p align="center">
<sub>© 2025 Wave Engine — Developed by Pablo Sanjose, Víctor González & Claudia Ruiz — MIT License</sub>
</p>
