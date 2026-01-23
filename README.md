# 🎮 Motor de Videojuegos - Sistema de Audio

## 📖 Introducción

Este proyecto es un motor de videojuegos 3D personalizado desarrollado en C++ utilizando OpenGL como API gráfica principal. Integra varias librerías externas como Assimp (para la carga de modelos 3D), DevIL (para la gestión de texturas) e ImGui (para la interfaz de usuario).

En la versión 2.0, hemos evolucionado más allá de la simple dependencia de FBX, implementando un sistema completo de gestión de recursos con formatos de archivo personalizados y optimizaciones significativas de renderizado. El motor ahora cuenta con una interfaz de editor avanzada con serialización de escenas, gestión de recursos y estructuras de aceleración para un rendimiento óptimo. 

Además, nuestro grupo ha desarrollado e integrado un **sistema de audio 3D completo** utilizando **Wwise** de Audiokinetic, que permite audio espacial, mezcla de música en loop y efectos de sonido dinámicos como el efecto túnel.

---

## 👥 Equipo de Desarrollo

<table align="center">
  <tr>
    <td align="center">
      <img src="https://avatars.githubusercontent.com/u/160216424?v=4" width="150" height="150"/><br/>
      <b>Arnau Pascual Delgado</b><br/>
      <a href="https://github.com/Pascra">Pascra</a>
    </td>
    <td align="center">
      <img src="https://avatars.githubusercontent.com/u/53059360?s=400&u=8b462a9616d8e479ef4beaf4657b9dd6be6662ee&v=4" width="150" height="150"/><br/>
      <b>Luis Fernández Díaz</b><br/>
      <a href="https://github.com/Ludef26">Ludef26</a>
    </td>
    <td align="center">
      <img src="https://avatars.githubusercontent.com/u/160216328?v=4" width="150" height="150"/><br/>
      <b>Roger Miralles Pellicero</b><br/>
      <a href="https://github.com/Roster247">Roster247</a>
    </td>
  </tr>
</table>

---

## 🔧 Sistemas Core del Motor

El motor está compuesto por los siguientes sistemas principales:

| Sistema | Descripción |
|---------|-------------|
| **Renderer** | Sistema de renderizado basado en OpenGL para gráficos 3D |
| **Scene Manager** | Gestión de escenas, GameObjects y jerarquía de entidades |
| **Transform** | Sistema de transformaciones (posición, rotación, escala) |
| **Physics** | Simulación física y detección de colisiones |
| **Input** | Gestión de entrada de teclado, ratón y gamepad |
| **Audio** | Sistema de audio 3D integrado con Wwise (nuestro sistema) |

---
## 🎮 Controles

### Controles Generales

| Acción | Tecla 1 | Tecla 2 |
|--------|---------|---------|
| Subir | Space | |
| Bajar | Left Ctrl | |
| Zoom | Rueda del ratón | |
| Velocidad ×2 | Mantener Shift | |
| Movimiento libre | Botón derecho del ratón | WASD |
| Orbitar | Left Alt | Botón izquierdo del ratón |
| Enfocar | F | |
| Seleccionar | Botón izquierdo del ratón | |
| Selección múltiple | Shift | Botón izquierdo del ratón |
| Eliminar objeto | Backspace | |

### Controles de Gizmo

| Acción | Tecla |
|--------|-------|
| Mover | W |
| Rotar | E |
| Escalar | R |
| Cambiar Sistema de Coordenadas | T |

### 🔊 Controles de Audio

| Acción | Tecla |
|--------|-------|
| Activar efecto de sonido envolvente | M |
| Activar sonido de túnel | T |
| Intensidad del túnel - Normal | 1 |
| Intensidad del túnel - Máxima | 2 |

---

## 🔊 Sistema de Audio - Implementación Detallada

### Arquitectura del Sistema

Nuestro sistema de audio está basado en la integración del motor de audio profesional **Wwise** de Audiokinetic. La arquitectura se compone de tres módulos principales:
```
┌─────────────────────────────────────────────────────────┐
│                      ModuleAudio                        │
│  - Inicialización/Terminación de Wwise                  │
│  - Gestión de Sound Banks                               │
│  - Control de GameObjects de audio                      │
│  - Parámetros RTPC (Real-Time Parameter Control)        │
└─────────────────────────────────────────────────────────┘
                           │
           ┌───────────────┴───────────────┐
           ▼                               ▼
┌─────────────────────┐         ┌─────────────────────┐
│ ComponentAudioSource│         │ComponentAudioListener│
│  - Emisor de sonido │         │  - Receptor de audio │
│  - Posición 3D      │         │  - Posición del      │
│  - Eventos de Wwise │         │    oyente            │
└─────────────────────┘         └─────────────────────┘
```

### Componentes Implementados

#### 1. ModuleAudio (`ModuleAudio.h` / `ModuleAudio.cpp`)

El módulo principal que gestiona toda la comunicación con Wwise:
```cpp
// Funcionalidades principales
- InitWwise()           // Inicialización del motor de audio
- RegisterAudioGameObject()  // Registro de objetos de audio
- SetGameObjectTransform()   // Posicionamiento 3D
- PostEvent()           // Reproducción de eventos de sonido
- SetRTPC()             // Control de parámetros en tiempo real
- SetListener()         // Configuración del listener
```

**Características:**
- Carga de Sound Banks desde memoria
- Gestión de múltiples GameObjects de audio
- Sistema de caché para parámetros RTPC
- Soporte para audio espacial 3D

#### 2. ComponentAudioSource (`ComponentAudioSource.h` / `ComponentAudioSource.cpp`)

Componente que permite a cualquier GameObject emitir sonido:
```cpp
// Uso básico
ComponentAudioSource(GameObject* owner, unsigned int eventId, bool playOnEnable);
```

**Características:**
- Registro automático en Wwise al habilitarse
- Actualización de posición 3D cada frame
- Reproducción automática al activarse (configurable)
- Limpieza automática al deshabilitarse

#### 3. ComponentAudioListener (`ComponentAudioListener.h` / `ComponentAudioListener.cpp`)

Componente que representa los "oídos" del jugador:

**Características:**
- Se sincroniza con la posición del GameObject padre
- Configura automáticamente el listener de Wwise
- Orientación del listener para audio direccional

---

### Funcionalidades Implementadas

#### 1. Sistema de Música con Mezcla

Implementación de dos pistas musicales que se mezclan en loop:

- Reproducción continua de música de fondo
- Transiciones suaves entre pistas
- Control de Play/Pause/Stop
- Gestión mediante eventos de Wwise (`MUSICSTARTA`)
```cpp
// Ejemplo de control de música
void ModuleAudio::OnPlay() {
    AK::SoundEngine::PostEvent(AK::EVENTS::MUSICSTARTA, MUSIC_GO);
}
```

#### 2. Audio Espacial 3D

GameObjects con audio posicional:

**GameObject Estático:**
- Posición fija en el mundo
- Sonido que se atenúa con la distancia
- Ejemplo: Sonido ambiental de una fuente

**GameObject Dinámico:**
- Movimiento automático o controlado por teclas (WASD)
- Audio que sigue la posición del objeto
- Demostración de efecto Doppler y atenuación espacial

<p align="center">
  <img src="images/audio_3d_diagram.gif" alt="Diagrama de Audio 3D" width="600"/>
</p>

#### 3. Tunnel Effect (En desarrollo)

Sistema de distorsión de audio por zonas:

- Definición de áreas mediante triggers
- Modificación de parámetros RTPC al entrar en zonas
- Efecto de reverberación/distorsión tipo túnel
```cpp
// Sistema preparado para RTPC
void SetRTPC(unsigned int rtpcId, float value, AkGameObjectID gameObjectId);
void SetRTPCByName(const char* rtpcName, float value, AkGameObjectID gameObjectId);
```
- Al parecer no se escucha en la escena a pesar de que todo el codigo y el sonido esta implementado en el wwise, no sabemos por qué ocurre esto.
---

## ✨ Extra Features

- Hemos añadido una **slider** en la interfaz del editor que permite controlar el volumen general de la música y los efectos de sonido en tiempo real.

---
## 🎬 Demostración

### Funcionamiento en el Editor

<p align="center">
  <img src="images/demo_editor.gif" alt="Demo en Editor" width="700"/>
  <br/>
  <i>Configuración de componentes de audio en el editor</i>
</p>

### Funcionamiento en Juego

<p align="center">
  <img src="images/demo_game.gif" alt="Demo en Juego" width="700"/>
  <br/>
  <i>Audio espacial 3D en tiempo de ejecución</i>
</p>

---

## 🎥 Video de Demostración

<p align="center">
  <a href="[LINK_AL_VIDEO]">
    <img src="images/video_thumbnail.png" alt="Video Demo" width="600"/>
  </a>
  <br/>
  <i>Click para ver el video de demostración (1 minuto en cámara rápida)</i>
</p>

**Contenido del video:**
1. Creación de una escena nueva
2. Añadir GameObjects con ComponentAudioSource
3. Configurar el AudioListener en la cámara
4. Demostración del audio 3D en ejecución
5. [Demostración del Tunnel Effect - si está implementado]

---

## 📥 Descarga y Enlaces

| Recurso | Enlace |
|---------|--------|
| 📂 **Repositorio** | [GitHub - Rama del proyecto](https://github.com/bottzo/Motor2025/tree/AudioEngine_LuisArnauRoger) |
| 📦 **Última Release** | [Descargar v1.0](https://github.com/[usuario]/[repo]/releases/tag/v1.0) |

---

## 📦 Estructura del Proyecto (Audio)
```
Motor2025/
├── Source/
│   ├── ModuleAudio.h
│   ├── ModuleAudio.cpp
│   ├── ComponentAudioSource.h
│   ├── ComponentAudioSource.cpp
│   ├── ComponentAudioListener.h
│   └── ComponentAudioListener.cpp
│
└── Audio/
    └── WwiseProject/
        └── MusicEngine/
            └── GeneratedSoundBanks/
                └── Windows/
                    ├── Init.bnk
                    └── Main.bnk
```

---

## 📄 Licencia

Este proyecto está licenciado bajo la **MITLicense**.
```
MIT License

Copyright (c) [year] [fullname]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

**Nota:** El motor de audio Wwise está sujeto a su propia licencia de Audiokinetic. 
Ver [Wwise Licensing](https://www.audiokinetic.com/licensing/) para más información.
