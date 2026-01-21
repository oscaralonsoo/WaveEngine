#type vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}

#type fragment
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;
uniform int hasTexture;
uniform vec3 materialDiffuse;
uniform vec3 lightDir;
uniform vec3 viewPos;

void main()
{
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * vec3(1.0);
  	
    // Diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(-lightDir);
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    
    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDirection + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * vec3(1.0);
        
    vec3 color = (ambient + diffuse + specular) * materialDiffuse;
    
    // --- Current Active Shader: Blinn-Phong ---
    if (hasTexture == 1) FragColor = vec4(color, 1.0) * texture(texture1, TexCoords);
    else FragColor = vec4(color, 1.0);

    // --- Alternative Variants (To use: uncomment one and comment the one above) ---
    
    // Normals: Shows the orientation of the faces as colors
    // FragColor = vec4(norm * 0.5 + 0.5, 1.0);
    
    // Grayscale: Converts the final image to black and white
    // float gray = dot(color, vec3(0.299, 0.587, 0.114));
    // FragColor = vec4(vec3(gray), 1.0);
    
    // Inverted: Flips all colors to their opposites
    // FragColor = vec4(1.0 - color, 1.0);
    
    // Unlit: Standard texture display without any shading from lights
    // if (hasTexture == 1) FragColor = texture(texture1, TexCoords);
    // else FragColor = vec4(materialDiffuse, 1.0);
    
    // Only Diffuse: Shows the base diffuse lighting without specular highlights
    // FragColor = vec4(diffuse * materialDiffuse, 1.0);
}
