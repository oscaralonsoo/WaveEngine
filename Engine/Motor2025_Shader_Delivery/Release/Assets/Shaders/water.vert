#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time; // Tu C++ ya está enviando esto correctamente

out vec3 FragPos;
out float WaveHeight; 

void main()
{
    // 1. Hacemos una copia de la posición original
    vec3 pos = aPos;
    
    // 2. CONFIGURACIÓN EXAGERADA (Para ver si se mueve)
    float frequency = 0.5;  // Qué tan anchas son las olas
    float amplitude = 0.2;  // Qué tan altas son
    float speed = 2.0;      // Velocidad
    
    // 3. Fórmula de la ola
    // Usamos el Tiempo enviado por tu C++
    float wave = sin(pos.x * frequency + time * speed) * amplitude;
    
    // Aplicamos la ola a la altura Y
    pos.y += wave;
    
    // 4. Salidas para el fragment shader
    WaveHeight = pos.y;
    FragPos = vec3(model * vec4(pos, 1.0));
    
    // --- AQUÍ ESTABA EL ERROR PROBABLEMENTE ---
    // Fíjate que usamos 'pos' (la modificada), NO 'aPos'
    gl_Position = projection * view * model * vec4(pos, 1.0);
}