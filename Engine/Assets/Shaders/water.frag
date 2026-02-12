#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in float WaveHeight; // Recibimos la altura de la ola

uniform float time; 

void main()
{
    // 1. Colores base
    vec3 deepWater = vec3(0.0, 0.05, 0.25); // Azul oscuro
    vec3 foamColor = vec3(1.0, 1.0, 1.0); // Blanco espuma
    
    // 2. Nivel espuma:
    float foamLevel = smoothstep(0.08, 0.17, WaveHeight);
    vec3 finalColor = mix(deepWater, foamColor, foamLevel);
    
    // 3. Transparencia
    FragColor = vec4(finalColor, 0.8); 
}