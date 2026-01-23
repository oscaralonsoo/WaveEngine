#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in float WaveHeight; // Recibimos la altura de la ola

// --- ESTA ES LA LÍNEA QUE TE FALTABA ---
uniform float time; 
// ---------------------------------------

void main()
{
    // 1. Colores base (Requisito: Azul abajo, blanco arriba)
    vec3 deepWater = vec3(0.0, 0.1, 0.5); // Azul oscuro
    vec3 foamColor = vec3(1.0, 1.0, 1.0); // Blanco espuma
    
    // 2. Interpolamos entre azul y blanco basándonos en la altura
    // Ajusta los valores -0.1 y 0.1 si ves mucha o poca espuma
    float foamLevel = smoothstep(-0.1, 0.1, WaveHeight);
    
    vec3 finalColor = mix(deepWater, foamColor, foamLevel);
    
    // 3. Transparencia (0.8) para cumplir el requisito de transparencias
    FragColor = vec4(finalColor, 0.8); 

    // --- PRUEBA DE TIEMPO ---
    // Compreueba si el uniform time funciona
    // FragColor = vec4(sin(time), 0.0, 0.0, 1.0);
}