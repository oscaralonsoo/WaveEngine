#include "Shaders.h"
#include <glad/glad.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include "Log.h"
#include <fstream>
#include <sstream>
#include <filesystem>


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
    auto ReadWholeFile = [](const std::string& path, std::string& out) -> bool
        {
            std::ifstream f(path, std::ios::in);
            if (!f.is_open()) return false;
            std::stringstream ss;
            ss << f.rdbuf();
            out = ss.str();
            return !out.empty();
        };

    std::string vsrcFile, fsrcFile;
    const std::string vpath = "Assets/Shaders/test.vert";
    const std::string fpath = "Assets/Shaders/test.frag";

    bool loadedFromFiles = ReadWholeFile(vpath, vsrcFile) && ReadWholeFile(fpath, fsrcFile);

    /*const char* vertexShaderSource = "#version 330 core\n"
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
        "   FragColor = vec4(result, alpha);\n"
        "}\0";*/

    const char* vertexShaderSource = nullptr;
    const char* fragmentShaderSource = nullptr;

    if (loadedFromFiles)
    {
        vertexShaderSource = vsrcFile.c_str();
        fragmentShaderSource = fsrcFile.c_str();
        LOG_CONSOLE("Using shader from files: %s + %s", vpath.c_str(), fpath.c_str());
    }
    else
    {
        // fallback: keep your original hardcoded strings
        /*vertexShaderSource = "...原本那段 #version 330 ...";
        fragmentShaderSource = "...原本那段 #version 330 ...";*/
         vertexShaderSource = "#version 330 core\n"
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

         fragmentShaderSource = "#version 330 core\n"
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
            "   FragColor = vec4(result, alpha);\n"
            "}\0";
        LOG_CONSOLE("Using built-in shader (shader files not found)");
    }


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

static bool CompileStage(GLuint stage, const char* src, std::string* outErr)
{
    glShaderSource(stage, 1, &src, nullptr);
    glCompileShader(stage);

    GLint ok = 0;
    glGetShaderiv(stage, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetShaderiv(stage, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(stage, len, nullptr, log.data());
        if (outErr) *outErr = log;
        return false;
    }
    return true;
}

bool Shader::CreateFromSource(const std::string& vertexSrc,
    const std::string& fragmentSrc,
    std::string* outErrorLog)
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

    std::string err;
    if (!CompileStage(vs, vertexSrc.c_str(), &err))
    {
        if (outErrorLog) *outErrorLog = "Vertex compile error:\n" + err;
        glDeleteShader(vs);
        glDeleteShader(fs);
        return false;
    }
    if (!CompileStage(fs, fragmentSrc.c_str(), &err))
    {
        if (outErrorLog) *outErrorLog = "Fragment compile error:\n" + err;
        glDeleteShader(vs);
        glDeleteShader(fs);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint len = 0;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(shaderProgram, len, nullptr, log.data());
        if (outErrorLog) *outErrorLog = "Link error:\n" + log;

        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return false;
    }

    return true;
}
