#version 330 core

// Atributos de entrada
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

// Variables Uniformes (Matrices)
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Salidas hacia el Fragment Shader
out vec3 FragPos;  // Posición en el mundo
out vec3 Normal;   // Normal de la superficie
out vec2 TexCoord; // Coordenadas de textura

void main()
{
    // Calculamos la posición del vértice en el mundo real
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Calculamos la normal corregida (importante si escalas objetos)
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    
    TexCoord = aTexCoord;
    
    // Posición final en pantalla
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}