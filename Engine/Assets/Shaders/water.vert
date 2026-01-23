#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time; 

out vec3 FragPos;
out float WaveHeight; // Enviamos la altura al fragment shader para el color

void main()
{
    vec3 pos = aPos;
    
    // --- AJUSTES PARA TU PLANO SIMPLE ---
    // Bajamos la frecuencia mucho (de 10.0 a 1.0 o menos)
    // para que la onda sea más grande que el propio plano.
    float frequency = 0.5; 
    
    // Bajamos la amplitud para que no sea tan exagerada
    float amplitude = 1.0; 
    
    // Fórmula de la ola
    float wave = sin(pos.x * frequency + time) * cos(pos.z * frequency + time);
    pos.y += wave * amplitude;
    
    WaveHeight = pos.y; 
    FragPos = vec3(model * vec4(pos, 1.0));
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}