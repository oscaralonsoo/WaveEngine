# Motor2025 – Shader Module

## Group

Shaders Team:

- Name1 Ao Yunqian  
- Name2 Hernández Vázquez Pau  

---

## What we implemented

We implemented a custom Shader system that allows:

### Ao Yunqian

- Build
- Loading .vert and .frag files from Assets
- Creating Shader resources
- Selecting shaders per Material in the Inspector
- Rendering objects using custom GPU programs

### Hernández Vázquez Pau

- Implementation of test.vert and test.frag with the *Blinn-Phong*
- Implementation of water.vert and water.frag
- Creation of a custom animated *wave* with the *water shader*
- A *test scene* with a plane already using the water shader
- *Transparency* on the animation related to the camera
- Vertex animation to simulate moving water
- Making the demo ready to be loaded directly without manual setup

---

## How to test

### Option A – Basic Shader Test

1. Open Engine.exe  
2. Load the default scene  
3. Select any building  
4. In Inspector → Material → Shader, choose `test.vert`  
5. The object will be rendered with a custom shader  

### Option B – Water Shader Demo (Pau)

1. Open Engine.exe  
2. Click **Load Scene**  
3. Load the **test scene**  
4. A plane with an animated water shader will appear  
5. You should see the water surface moving automatically  

This scene already contains the water plane, so no manual creation is needed.

---

## Files

Assets/Shaders/test.vert  
Assets/Shaders/test.frag  

Assets/Shaders/water.vert  
Assets/Shaders/water.frag  

Scene/TestScene (water shader demo scene)

These files demonstrate custom vertex and fragment shaders, including an animated water effect.




