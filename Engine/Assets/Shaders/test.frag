#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// Uniforms
uniform sampler2D texture1; // La textura de la casa
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
// uniform vec3 objectColor; // Ya no lo usamos, usamos la textura

void main()
{
    // --- PARÁMETROS DE LUZ (Descomenta si el motor no envía luz aún) ---
    // vec3 lightPos = vec3(10.0, 30.0, 10.0);
    // vec3 viewPos = vec3(0.0, 5.0, -10.0);
    // vec3 lightColor = vec3(1.0, 1.0, 1.0);

    // 1. Obtener el color de la textura en este píxel
    vec4 texColor = texture(texture1, TexCoord);

    // 2. AMBIENTE
    float ambientStrength = 0.3; // Subimos un poco para que no se vea muy oscuro
    vec3 ambient = ambientStrength * lightColor;
  
    // 3. DIFUSA (Lambert)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 4. ESPECULAR (Blinn-Phong)
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;  
        
    // RESULTADO FINAL: (Luz) * (Color Textura)
    vec3 lighting = (ambient + diffuse + specular);
    vec3 result = lighting * texColor.rgb;
    
    FragColor = vec4(result, texColor.a);
}