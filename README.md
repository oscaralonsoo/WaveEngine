# Engine
Texto introduccion

***
### Team members:
* Haosheng Li: https://github.com/HaosLii 
* Ana Alcaraz: https://github.com/Audra0000
***

## Camera controls:
| Action | Key 1 | Key 2 |
|------------|------------|------------|
| Up         | Space      |            |
| Down       | L Ctrl     |            |
| Zoom       | Mouse wheel|            |
| Velocity * 2 | Shift held down |     |
| Free around| RMB        | WASD       |
| Orbit      | L Alt      | LMB        |
| Focus      | F          |            |
| Select     |LMB         |            |
| Deselect   |            |            |
| Multiple select| Shift  |  LMB       |

***

## UI:
* Console: This window logs the process of loading geometry
from ASSIMP, textures from Devil, the initialisation of external libraries, application flow
and possible errors.
* Configuration: FPS, variables de cada modulo, informacion consumo de memoria.
* Hierarchy: 
* Inspector:
  * Transform: Displays the position, rotation, and scale of the
selected GameObject.
  * Mesh: Information about the loaded mesh. Including an option to visualise the normals (per-triangle and per-face).
  * Texture: Displays the path to the texture and its size. Includes an option to
visualise the texture of the black and white squares on the selected GameObject
* Tools bar: It is a toolbar that acts as a general menu, including the following options:
  *  File: Option to exit the program
  *  View: Shows/Hides the available windows of the engine editor
  *  Help: Includes the following options:
    * GitHub documentation: Opens the browser and takes you to the documentation
    * Report a bug: Opens the browser and takes you to: [Link to repo]/issues
    * Download latest: Opens the browser and takes you to: [Link to repo]/releases
    * About: Displays a window with the name of the engine + version + team members + libraries used + MIT Licence.  

***
### Extra features:
* Selection
* Ability to modify inspector values
