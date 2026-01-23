# Wave Engine 🌊

**Wave Engine** es un motor de videojuegos en desarrollo creado como proyecto para la asignatura de **Motores de Videojuegos** en el **CITM - UPC (Terrassa)**. El motor está diseñado para ofrecer un flujo de trabajo intuitivo integrando un sistema de físicas avanzado basado en la industria.

---

## 👥 El Equipo

| Miembro | Contribuciones principales |
| :--- | :--- |
| **Toni Llovera Roca** | Implementación del **Vehicle Controller** (física de ruedas y chasis) y sistema de asignación automática de **RigidBodies y Colliders** para primitivas y modelos 3D. |
| **Javier Gómez González** | Desarrollo del sistema de **Cámara**, arquitectura de clases de física, **Module Physics** y el componente base **Collider**. |
| **Oscar Alonso Camenforte** | Integración de la librería **Bullet Physics** y desarrollo del sistema de **Point-to-Point Constraints** configurable desde el editor. |

---

## 🛠️ Core Systems (Sistemas Base)

Aunque el enfoque de esta entrega son las físicas, el **Wave Engine** se sustenta sobre los siguientes pilares:

* **Render Engine:** Basado en **OpenGL**, permite la visualización de geometrías complejas, texturas y el "Debug Draw" de las físicas.
* **Physics Core (Bullet):** Integración profunda de la librería Bullet para la gestión de la simulación, detección de colisiones y resolución de restricciones.
* **Input System (SDL):** Gestión de estados de teclado y ratón para el control del vehículo y la cámara.
* **Editor UI (ImGui):** Suite de herramientas que incluye el Inspector de componentes, explorador de archivos y consola de logs.

---

## 🚀 High-Level System: Advanced Physics & Vehicle Simulation

Siguiendo los requerimientos de la asignatura, hemos implementado un sistema de alto nivel que permite crear gameplay basado en físicas de forma modular.

### Características detalladas:
1.  **Vehicle Physics:** Un controlador de vehículo que utiliza mallas gráficas para las ruedas y el chasis, con parámetros de suspensión y motor ajustables.
2.  **Point-to-Point Constraints:** En el **Nivel 2**, se encuentran dos primitivas enlazadas. Este sistema permite crear uniones físicas tipo "péndulo" o articulaciones configurables directamente desde el Inspector.
3.  **Automatic Physics Assignment:** Al importar un asset o crear una primitiva, el motor genera automáticamente su contraparte física (RigidBody + Collider) ajustando el volumen inicial al tamaño del objeto.

> ### 💡 Guía de Testing: Cámara y Colliders
> Para verificar el **Collider de la Cámara** y su comportamiento físico:
> 1. Pulsa el botón **Play** en la parte superior del editor.
> 2. Una vez en juego, cambia manualmente de la pestaña **Game** a la pestaña **Scene**.
> 3. Utiliza la combinación **WASD + Click Derecho mantenido** para moverte. Verás cómo la cámara interactúa con el mundo físico.


---

## 📽️ Scene Creation Timelapse

En este vídeo de 1 minuto se muestra el proceso de diseño de una escena, configurando los componentes físicos y los constraints desde cero:

[![Watch the video](https://drive.google.com/file/d/1QPZXN4Q8vPpQJ6Qza2XsVvcpFhlluP_G/view?usp=sharing)]

---

## 🔗 Enlaces de Interés

* **Repositorio:** [Wave Engine - Physics_Group3](https://github.com/bottzo/Motor2025/tree/Physics_Group3)
* **Última Release:** [Descargar Wave Engine v1.0](PONER_LINK_A_LA_RELEASE_AQUÍ)
