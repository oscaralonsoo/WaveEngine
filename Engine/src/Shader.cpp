#include "Shader.h"
#include <glad/glad.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include "Log.h"

Shader::Shader() : shaderProgram(0)
{
}

Shader::~Shader()
{
    Delete();
}

bool Shader::Create()
{
    // Standard vertex shader with MVP transformation
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "\n"
        "out vec2 TexCoord;\n"
        "\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   TexCoord = aTexCoord;\n"
        "}\0";

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Vertex Shader Compilation Failed\n" << infoLog << std::endl;
        return false;
    }

    // Fragment shader with alpha discard for vegetation/cutout textures
    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D texture1;\n"
        "uniform vec3 tintColor;\n"
        "void main()\n"
        "{\n"
        "   vec4 texColor = texture(texture1, TexCoord);\n"
        "   \n"
        "   // Discard fully transparent pixels (grass, leaves, etc.)\n"
        "   if(texColor.a < 0.01)\n"
        "       discard;\n"
        "   \n"
        "   FragColor = vec4(texColor.rgb * tintColor, texColor.a);\n"
        "}\0";

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Fragment Shader Compilation Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }

    // Link shaders into program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader Program Linking Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Clean up shader objects (no longer needed after linking)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    LOG_CONSOLE("Shader created successfully!");

    return true;
}

void Shader::Use() const
{
    glUseProgram(shaderProgram);
}

bool Shader::LoadFromSource(const char* vSource, const char* fSource, const char* gSource)
{
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vSource);
    if (vertexShader == 0) return false;

    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    unsigned int geometryShader = 0;
    if (gSource != nullptr) {
        geometryShader = CompileShader(GL_GEOMETRY_SHADER, gSource);
        if (geometryShader == 0) {
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return false;
        }
    }

    unsigned int newProgram = glCreateProgram();
    glAttachShader(newProgram, vertexShader);
    glAttachShader(newProgram, fragmentShader);
    if (geometryShader != 0) glAttachShader(newProgram, geometryShader);
    glLinkProgram(newProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(newProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(newProgram, 512, NULL, infoLog);
        LOG_CONSOLE("ERROR: Shader Program Linking Failed\n%s", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryShader != 0) glDeleteShader(geometryShader);
        glDeleteProgram(newProgram);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (geometryShader != 0) glDeleteShader(geometryShader);

    // If there was an old program, delete it
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
    }

    shaderProgram = newProgram;
    return true;
}

unsigned int Shader::CompileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        const char* typeStr = (type == GL_VERTEX_SHADER) ? "Vertex" : (type == GL_FRAGMENT_SHADER ? "Fragment" : "Geometry");
        LOG_CONSOLE("ERROR: %s Shader Compilation Failed\n%s", typeStr, infoLog);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void Shader::Delete()
{
    if (shaderProgram != 0)
    {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, &value[0]);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(shaderProgram, name.c_str()), value);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

bool Shader::CreateSimpleColor()
{
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoords;\n"
        "\n"
        "out vec2 TexCoords;\n"
        "\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   TexCoords = aTexCoords;\n"
        "}\0";

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Simple Vertex Shader Compilation Failed\n" << infoLog << std::endl;
        return false;
    }

    // Fragment shader for semi-transparent surfaces (windows, glass, etc.)
    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoords;\n"
        "\n"
        "uniform sampler2D texture1;\n"
        "uniform vec3 tintColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   vec4 texColor = texture(texture1, TexCoords);\n"
        "   \n"
        "   FragColor = vec4(texColor.rgb * tintColor, texColor.a);\n"
        "}\0";

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Simple Fragment Shader Compilation Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    LOG_CONSOLE("Simple color shader created successfully!");

    return true;
}

bool Shader::CreateWithDiscard()
{
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "\n"
        "out vec2 TexCoord;\n"
        "\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   TexCoord = aTexCoord;\n"
        "}\0";

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Discard Vertex Shader Compilation Failed\n" << infoLog << std::endl;
        return false;
    }

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D texture1;\n"
        "uniform vec3 tintColor;\n"
        "void main()\n"
        "{\n"
        "   vec4 texColor = texture(texture1, TexCoord);\n"
        "   \n"
        "   if(texColor.a < 0.1)\n"
        "       discard;\n"
        "   \n"
        "   FragColor = vec4(texColor.rgb * tintColor, texColor.a);\n"
        "}\0";

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Discard Fragment Shader Compilation Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    LOG_CONSOLE("Discard shader created successfully!");

    return true;
}

bool Shader::CreateSingleColor()
{

    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "uniform float outlineThickness;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   vec3 scale = vec3(\n"
        "       length(model[0].xyz),\n"
        "       length(model[1].xyz),\n"
        "       length(model[2].xyz)\n"
        "   );\n"
        "   mat4 modelNoScale = model;\n"
        "   modelNoScale[0].xyz /= scale.x;\n"
        "   modelNoScale[1].xyz /= scale.y;\n"
        "   modelNoScale[2].xyz /= scale.z;\n"
        "   \n"
        "   vec3 worldNormal = normalize(mat3(modelNoScale) * aNormal);\n"
        "   \n"
        "   vec4 worldPos = model * vec4(aPos, 1.0);\n"
        "   \n"
        "   vec4 viewPos = view * worldPos;\n"
        "   float distance = length(viewPos.xyz);\n"
        "   float dynamicThickness = outlineThickness * (distance * 0.1);\n"
        "   \n"
        "   worldPos.xyz += worldNormal * dynamicThickness;\n"
        "   \n"
        "   gl_Position = projection * view * worldPos;\n"
        "}\0";

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Outline Vertex Shader Compilation Failed\n" << infoLog << std::endl;
        return false;
    }

    // Solid color fragment shader for selection outlines
    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "\n"
        "uniform vec3 outlineColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(outlineColor, 1.0);\n"
        "}\0";

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Outline Fragment Shader Compilation Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    LOG_CONSOLE("Outline shader created successfully!");

    return true;
}

bool Shader::CreateDepthVisualization()
{
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "\n"
        "out vec2 TexCoord;\n"
        "\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   TexCoord = aTexCoord;\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "\n"
        "uniform float nearPlane;\n"
        "uniform float farPlane;\n"
        "\n"
        "float LinearizeDepth(float depth)\n"
        "{\n"
        "    float z = depth * 2.0 - 1.0; // back to NDC\n"
        "    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));\n"
        "}\n"
        "\n"
        "void main()\n"
        "{\n"
        "    float depth = LinearizeDepth(gl_FragCoord.z) / farPlane;\n"
        "    FragColor = vec4(vec3(depth), 1.0);\n"
        "}\0";

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Depth Vertex Shader Compilation Failed\n" << infoLog << std::endl;
        return false;
    }

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Depth Fragment Shader Compilation Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Depth Shader Program Linking Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    LOG_CONSOLE("Depth visualization shader created successfully!");

    return true;
}

bool Shader::CreateNoTexture()
{
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "\n"
        "out vec3 FragPos;\n"
        "out vec3 Normal;\n"
        "out vec2 TexCoord;\n"
        "\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
        "   TexCoord = aTexCoord;\n"
        "   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "\n"
        "in vec3 FragPos;\n"
        "in vec3 Normal;\n"
        "in vec2 TexCoord;\n"
        "\n"
        "uniform sampler2D texture1;\n"
        "uniform int hasTexture;\n"
        "uniform vec3 tintColor;\n"
        "uniform vec3 lightDir;\n"
        "uniform vec3 materialDiffuse;\n"
        "uniform float opacity;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   vec3 baseColor;\n"
        "   float alpha = 1.0;\n"
        "   \n"
        "   if (hasTexture == 1) {\n"
        "       vec4 texColor = texture(texture1, TexCoord);\n"
        "       if(texColor.a < 0.01) discard;\n"
        "       baseColor = texColor.rgb * tintColor;\n"
        "       alpha = texColor.a;\n"
        "   } else {\n"
        "       baseColor = materialDiffuse;\n"
        "   }\n"
        "   \n"
        "   // Iluminaci?n difusa simple\n"
        "   vec3 norm = normalize(Normal);\n"
        "   vec3 light = normalize(-lightDir);\n"
        "   float diff = max(dot(norm, light), 0.0);\n"
        "   \n"
        "   // Luz ambiental + difusa\n"
        "   vec3 ambient = 0.3 * baseColor;\n"
        "   vec3 diffuse = diff * baseColor;\n"
        "   \n"
        "   vec3 result = ambient + diffuse;\n"
        "   FragColor = vec4(result, alpha * opacity);\n"
        "}\0";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        return false;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: NoTexture Fragment Shader Compilation Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: NoTexture Shader Program Linking Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    return true;
}

bool Shader::CreateWater()
{
    // 1. Vertex Shader: Passthrough (Geometry shader will handle displacement)
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "\n"
        "out VS_OUT {\n"
        "    vec3 pos;\n"
        "    vec3 normal;\n"
        "    vec2 texCoord;\n"
        "} vs_out;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vs_out.pos = aPos;\n"
        "    vs_out.normal = aNormal;\n"
        "    vs_out.texCoord = aTexCoord;\n"
        "}\0";

    // 2. Geometry Shader: Dynamic Tessellation & Wave Calculation
    const char* geometryShaderSource = "#version 330 core\n"
        "layout (triangles) in;\n"
        "layout (triangle_strip, max_vertices = 12) out;\n"
        "\n"
        "in VS_OUT {\n"
        "    vec3 pos;\n"
        "    vec3 normal;\n"
        "    vec2 texCoord;\n"
        "} gs_in[];\n"
        "\n"
        "out vec3 FragPos;\n"
        "out vec3 Normal;\n"
        "out vec2 TexCoord;\n"
        "out float v_Height;\n"
        "\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "uniform float u_Time;\n"
        "\n"
        "uniform float waveSpeed;\n"
        "uniform float waveAmplitude;\n"
        "uniform float waveFrequency;\n"
        "\n"
        "struct Wave {\n"
        "    vec2 direction;\n"
        "    float steepness;\n"
        "    float wavelength;\n"
        "};\n"
        "\n"
        "vec3 GerstnerWave(vec3 p, Wave w, float time, inout vec3 tangent, inout vec3 binormal)\n"
        "{\n"
        "    float k = 2.0 * 3.14159 / w.wavelength;\n"
        "    float c = sqrt(9.8 / k);\n"
        "    vec2 d = normalize(w.direction);\n"
        "    float f = k * (dot(d, p.xz) - c * time * waveSpeed);\n"
        "    float a = (w.steepness / k) * waveAmplitude;\n"
        "    \n"
        "    float cosf = cos(f);\n"
        "    float sinf = sin(f);\n"
        "    \n"
        "    vec3 disp = vec3(\n"
        "        d.x * (a * cosf),\n"
        "        a * sinf,\n"
        "        d.y * (a * cosf)\n"
        "    );\n"
        "    \n"
        "    float wa = k * a;\n"
        "    tangent += vec3(\n"
        "        -d.x * d.x * (wa * sinf),\n"
        "        d.x * (wa * cosf),\n"
        "        -d.x * d.y * (wa * sinf)\n"
        "    );\n"
        "    \n"
        "    binormal += vec3(\n"
        "        -d.x * d.y * (wa * sinf),\n"
        "        d.y * (wa * cosf),\n"
        "        -d.y * d.y * (wa * sinf)\n"
        "    );\n"
        "    \n"
        "    return disp;\n"
        "}\n"
        "\n"
        "vec3 ApplyWaves(vec3 gridPoint, out vec3 normal, out float height)\n"
        "{\n"
        "    vec3 finalPos = gridPoint;\n"
        "    vec3 tangent = vec3(1.0, 0.0, 0.0);\n"
        "    vec3 binormal = vec3(0.0, 0.0, 1.0);\n"
        "    \n"
        "    // Reduced default steepness for smoother look\n"
        "    Wave w1 = Wave(vec2(1.0, 0.5), 0.15, 6.0 / waveFrequency);\n"
        "    Wave w2 = Wave(vec2(0.6, 1.0), 0.10, 3.5 / waveFrequency);\n"
        "    Wave w3 = Wave(vec2(-0.4, 0.8), 0.10, 2.0 / waveFrequency);\n"
        "    \n"
        "    finalPos += GerstnerWave(gridPoint, w1, u_Time, tangent, binormal);\n"
        "    finalPos += GerstnerWave(gridPoint, w2, u_Time, tangent, binormal);\n"
        "    finalPos += GerstnerWave(gridPoint, w3, u_Time, tangent, binormal);\n"
        "    \n"
        "    normal = normalize(cross(binormal, tangent));\n"
        "    height = finalPos.y;\n"
        "    \n"
        "    return finalPos;\n"
        "}\n"
        "\n"
        "void EmitVertexWithWave(vec3 pos, vec2 tex)\n"
        "{\n"
        "    vec3 finalNormal;\n"
        "    float h;\n"
        "    vec3 wavePos = ApplyWaves(pos, finalNormal, h);\n"
        "    \n"
        "    v_Height = h;\n"
        "    FragPos = vec3(model * vec4(wavePos, 1.0));\n"
        "    Normal = normalize(mat3(transpose(inverse(model))) * finalNormal);\n"
        "    TexCoord = tex;\n"
        "    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
        "    EmitVertex();\n"
        "}\n"
        "\n"
        "void main()\n"
        "{\n"
        "    // Simple subdivision: 1 triangle -> 4 triangles\n"
        "    //      v2\n"
        "    //      /\\\n"
        "    //  m3 /__\\ m2\n"
        "    //    /\\  /\\\n"
        "    // v0/__\\/__\\v1\n"
        "    //      m1\n"
        "\n"
        "    vec3 v0 = gs_in[0].pos;\n"
        "    vec3 v1 = gs_in[1].pos;\n"
        "    vec3 v2 = gs_in[2].pos;\n"
        "\n"
        "    vec3 m1 = (v0 + v1) / 2.0;\n"
        "    vec3 m2 = (v1 + v2) / 2.0;\n"
        "    vec3 m3 = (v2 + v0) / 2.0;\n"
        "\n"
        "    vec2 t0 = gs_in[0].texCoord;\n"
        "    vec2 t1 = gs_in[1].texCoord;\n"
        "    vec2 t2 = gs_in[2].texCoord;\n"
        "\n"
        "    vec2 tm1 = (t0 + t1) / 2.0;\n"
        "    vec2 tm2 = (t1 + t2) / 2.0;\n"
        "    vec2 tm3 = (t2 + t0) / 2.0;\n"
        "\n"
        "    // Triangle 1 (Bottom Left)\n"
        "    EmitVertexWithWave(v0, t0);\n"
        "    EmitVertexWithWave(m1, tm1);\n"
        "    EmitVertexWithWave(m3, tm3);\n"
        "    EndPrimitive();\n"
        "\n"
        "    // Triangle 2 (Top)\n"
        "    EmitVertexWithWave(m3, tm3);\n"
        "    EmitVertexWithWave(m2, tm2);\n"
        "    EmitVertexWithWave(v2, t2);\n"
        "    EndPrimitive();\n"
        "\n"
        "    // Triangle 3 (Bottom Right)\n"
        "    EmitVertexWithWave(m1, tm1);\n"
        "    EmitVertexWithWave(v1, t1);\n"
        "    EmitVertexWithWave(m2, tm2);\n"
        "    EndPrimitive();\n"
        "\n"
        "    // Triangle 4 (Center)\n"
        "    EmitVertexWithWave(m1, tm1);\n"
        "    EmitVertexWithWave(m2, tm2);\n"
        "    EmitVertexWithWave(m3, tm3);\n"
        "    EndPrimitive();\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "\n"
        "in vec3 FragPos;\n"
        "in vec3 Normal;\n"
        "in vec2 TexCoord;\n"
        "in float v_Height;\n"
        "\n"
        "uniform vec3 lightDir;\n"
        "uniform vec3 viewPos;\n"
        "uniform float waveAmplitude;\n"
        "uniform float opacity;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec3 viewDir = normalize(viewPos - FragPos);\n"
        "    vec3 norm = normalize(Normal);\n"
        "    vec3 light = normalize(-lightDir);\n"
        "    vec3 halfwayDir = normalize(light + viewDir);\n"
        "    \n"
        "    vec3 deepColor = vec3(0.0, 0.1, 0.4);\n"
        "    vec3 shallowColor = vec3(0.0, 0.4, 0.6);\n"
        "    vec3 foamColor = vec3(1.0, 1.0, 1.0);\n"
        "    \n"
        "    // Adjust height factor based on amplitude\n"
        "    float heightFactor = smoothstep(-0.5 * waveAmplitude, 0.8 * waveAmplitude, v_Height);\n"
        "    vec3 albedo = mix(deepColor, shallowColor, heightFactor);\n"
        "    \n"
        "    float foamFactor = smoothstep(0.4 * waveAmplitude, 0.6 * waveAmplitude, v_Height);\n"
        "    albedo = mix(albedo, foamColor, foamFactor);\n"
        "    \n"
        "    vec3 ambient = vec3(0.1) * albedo;\n"
        "    float diff = max(dot(norm, light), 0.0);\n"
        "    vec3 diffuse = diff * albedo;\n"
        "    \n"
        "    float spec = pow(max(dot(norm, halfwayDir), 0.0), 128.0);\n"
        "    vec3 specular = vec3(0.8) * spec;\n"
        "    \n"
        "    float F0 = 0.02;\n"
        "    float fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(norm, viewDir), 0.0), 5.0);\n"
        "    \n"
    "    vec3 result = ambient + diffuse + specular;\n"
        "    float alpha = mix(0.7, 0.95, fresnel);\n"
        "    \n"
        "    FragColor = vec4(result, alpha * opacity);\n"
        "}\0";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Water Vertex Shader Compilation Failed\n" << infoLog << std::endl;
        return false;
    }

    // Geometry Shader compilation
    unsigned int geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometryShader, 1, &geometryShaderSource, NULL);
    glCompileShader(geometryShader);

    glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(geometryShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Water Geometry Shader Compilation Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR: Water Fragment Shader Compilation Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(geometryShader);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, geometryShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR: Water Shader Program Linking Failed\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(geometryShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    glDeleteShader(fragmentShader);

    return true;
}

void Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), value);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, &value[0]);
}

void Shader::SetBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), (int)value);
}