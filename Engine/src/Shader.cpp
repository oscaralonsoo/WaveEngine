#include "Shader.h"
#include <glad/glad.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include "Log.h"

Shader::Shader() : shaderProgram(0)
{
    shaderHeader =
        "#version 460 core\n"
        "layout(std140, binding = 0) uniform Matrices {\n"
        "    mat4 view;\n"
        "    mat4 projection;\n"
        "};\n";

    skinningDeclarations =
        "layout(std430, binding = 0) readonly buffer BoneMatrices { mat4 gBones[]; };\n"
        "layout(std430, binding = 1) readonly buffer OffsetMatrices { mat4 gOffsets[]; };\n"
        "uniform mat4 meshInverse;\n"
        "uniform bool hasBones;\n"
        "uniform mat4 model;\n";

    skinningFunction =
        "mat4 GetSkinMatrix(ivec4 ids, vec4 weights) {\n"
        "    if (!hasBones) return mat4(1.0);\n"
        "    mat4 skinMat = mat4(0.0);\n"
        "    float weightSum = weights.x + weights.y + weights.z + weights.w;\n"
        "    if (weightSum < 0.001) return mat4(1.0);\n"
        "    for(int i = 0; i < 4; i++) {\n"
        "        if(ids[i] == -1) continue;\n"
        "        mat4 boneTransform = meshInverse * gBones[ids[i]] * gOffsets[ids[i]];\n"
        "        skinMat += boneTransform * (weights[i] / weightSum);\n"
        "    }\n"
        "    return skinMat;\n"
        "}\n";
}

Shader::~Shader()
{
    Delete();
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
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[4096];
        glGetShaderInfoLog(shader, 4096, NULL, infoLog);
        const char* typeStr = (type == GL_VERTEX_SHADER) ? "Vertex" :
            (type == GL_FRAGMENT_SHADER ? "Fragment" : "Geometry");
        LOG_CONSOLE("ERROR: %s Shader Compilation Failed:\n%s", typeStr, infoLog);

        LOG_CONSOLE("Source:\n%s", source);

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
    std::string vert = std::string(shaderHeader) + skinningDeclarations + skinningFunction +
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoords;\n"
        "layout(location = 3) in ivec4 boneIDs;\n"
        "layout(location = 4) in vec4 weights;\n"
        "out vec2 TexCoords;\n"
        "void main() {\n"
        "    mat4 skinMat = GetSkinMatrix(boneIDs, weights);\n"
        "    vec4 skinnedPos = skinMat * vec4(aPos, 1.0);\n"
        "    gl_Position = projection * view * model * skinnedPos;\n"
        "    TexCoords = aTexCoords;\n"
        "}\n";

    std::string frag =
        "#version 460 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoords;\n"
        "uniform sampler2D texture1;\n"
        "uniform vec3 tintColor;\n"
        "void main() {\n"
        "    vec4 texColor = texture(texture1, TexCoords);\n"
        "    FragColor = vec4(texColor.rgb * tintColor, texColor.a);\n"
        "}\n";

    return LoadFromSource(vert.c_str(), frag.c_str());
}

bool Shader::CreateNormalShader()
{
    std::string vert = std::string(shaderHeader) + skinningDeclarations + skinningFunction +
        "layout (location = 0) in vec3 position;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "layout (location = 3) in ivec4 boneIDs;\n"
        "layout (location = 4) in vec4 weights;\n"
        "\n"
        "out VS_OUT { vec3 normal; } vs_out;\n"
        "\n"
        "void main() {\n"
        "    mat4 skinMat = GetSkinMatrix(boneIDs, weights);\n"
        "    vec4 skinnedPos = skinMat * vec4(position, 1.0f);\n"
        "    \n"
        "    // Usamos la normal transformada por el skinning\n"
        "    vs_out.normal = normalize(mat3(skinMat) * aNormal);\n"
        "    gl_Position = skinnedPos;\n"
        "}\n";

    std::string geom = std::string(shaderHeader) +
        "layout (points) in;\n"
        "layout (line_strip, max_vertices = 2) out;\n"
        "in VS_OUT { vec3 normal; } gs_in[];\n"
        "\n"
        "uniform mat4 model;\n"
        "const float LINE_LENGTH = 0.3;\n"
        "\n"
        "void main() {\n"
        "    // 1. Transformamos la normal a espacio de mundo (usando solo la rotación del modelo)\n"
        "    // Evitamos inverse() para mayor estabilidad\n"
        "    vec3 worldNormal = normalize(mat3(model) * gs_in[0].normal);\n"
        "    \n"
        "    // 2. Transformamos la posición a espacio de mundo\n"
        "    vec4 worldPos = model * gl_in[0].gl_Position;\n"
        "\n"
        "    // Punto inicial: El vértice\n"
        "    gl_Position = projection * view * worldPos;\n"
        "    EmitVertex();\n"
        "\n"
        "    // Punto final: Vértice + normal estirada\n"
        "    gl_Position = projection * view * (worldPos + vec4(worldNormal * LINE_LENGTH, 0.0));\n"
        "    EmitVertex();\n"
        "\n"
        "    EndPrimitive();\n"
        "}\n";

    std::string frag =
        "#version 460 core\n"
        "out vec4 color;\n"
        "uniform vec4 lineColor;\n"
        "void main() { color = lineColor; }\n";

    return LoadFromSource(vert.c_str(), frag.c_str(), geom.c_str());
}

bool Shader::CreateMeshShader()
{
    std::string vert = std::string(shaderHeader) + skinningDeclarations + skinningFunction +
        "layout (location = 0) in vec3 position;\n"
        "layout (location = 3) in ivec4 boneIDs;\n"
        "layout (location = 4) in vec4 weights;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    mat4 skinMat = GetSkinMatrix(boneIDs, weights);\n"
        "    vec4 skinnedPos = skinMat * vec4(position, 1.0f);\n"
        "\n"
        "    gl_Position = projection * view * model * skinnedPos;\n"
        "}\n";

    std::string frag =
        "#version 460 core\n"
        "out vec4 FragColor;\n"
        "uniform vec4 lineColor;\n"
        "void main() {\n"
        "    FragColor = lineColor;\n"
        "}\n";

    return LoadFromSource(vert.c_str(), frag.c_str());
}

bool Shader::CreateSingleColor()
{
    std::string vert = std::string(shaderHeader) + skinningDeclarations + skinningFunction +
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 3) in ivec4 boneIDs;\n"
        "layout(location = 4) in vec4 weights;\n"
        "uniform float outlineThickness;\n"
        "void main() {\n"
        "    mat4 skinMat = GetSkinMatrix(boneIDs, weights);\n"
        "    vec4 skinnedPos = skinMat * vec4(aPos, 1.0);\n"
        "    vec3 skinnedNormal = normalize(mat3(skinMat) * aNormal);\n"
        "    vec3 scale = vec3(length(model[0].xyz), length(model[1].xyz), length(model[2].xyz));\n"
        "    mat4 modelNoScale = model;\n"
        "    modelNoScale[0].xyz /= scale.x;\n"
        "    modelNoScale[1].xyz /= scale.y;\n"
        "    modelNoScale[2].xyz /= scale.z;\n"
        "    vec3 worldNormal = normalize(mat3(modelNoScale) * skinnedNormal);\n"
        "    vec4 worldPos = model * skinnedPos;\n"
        "    vec4 viewPos = view * worldPos;\n"
        "    float distance = length(viewPos.xyz);\n"
        "    float dynamicThickness = outlineThickness * (distance * 0.1);\n"
        "    worldPos.xyz += worldNormal * dynamicThickness;\n"
        "    gl_Position = projection * view * worldPos;\n"
        "}\n";

    std::string frag =
        "#version 460 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 outlineColor;\n"
        "void main() {\n"
        "    FragColor = vec4(outlineColor, 1.0);\n"
        "}\n";

    return LoadFromSource(vert.c_str(), frag.c_str());
}

bool Shader::CreateDepthVisualization()
{
    std::string vert = std::string(shaderHeader) + skinningDeclarations + skinningFunction +
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoord;\n"
        "layout(location = 3) in ivec4 boneIDs;\n"
        "layout(location = 4) in vec4 weights;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "    mat4 skinMat = GetSkinMatrix(boneIDs, weights);\n"
        "    vec4 skinnedPos = skinMat * vec4(aPos, 1.0);\n"
        "    gl_Position = projection * view * model * skinnedPos;\n"
        "    TexCoord = aTexCoord;\n"
        "}\n";

    std::string frag =
        "#version 460 core\n"
        "out vec4 FragColor;\n"
        "uniform float nearPlane;\n"
        "uniform float farPlane;\n"
        "float LinearizeDepth(float depth) {\n"
        "    float z = depth * 2.0 - 1.0;\n"
        "    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));\n"
        "}\n"
        "void main() {\n"
        "    float depth = LinearizeDepth(gl_FragCoord.z) / farPlane;\n"
        "    FragColor = vec4(vec3(depth), 1.0);\n"
        "}\n";

    return LoadFromSource(vert.c_str(), frag.c_str());
}

bool Shader::CreateLinesShader()
{
    std::string vert = std::string(shaderHeader) +
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec4 aColor;\n"
        "out vec4 ourColor;\n"
        "uniform mat4 model;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "    ourColor = aColor;\n"
        "}\n";

    std::string frag =
        "#version 460 core\n"
        "out vec4 FragColor;\n"
        "in vec4 ourColor;\n"
        "void main() {\n"
        "    FragColor = ourColor;\n"
        "}\n";

    return LoadFromSource(vert.c_str(), frag.c_str());
}

bool Shader::CreateNoTexture()
{
    std::string vert = std::string(shaderHeader) + skinningDeclarations + skinningFunction +
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoord;\n"
        "layout(location = 3) in ivec4 boneIDs;\n"
        "layout(location = 4) in vec4 weights;\n"
        "out vec3 FragPos;\n"
        "out vec3 Normal;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "    mat4 skinMat = GetSkinMatrix(boneIDs, weights);\n"
        "    vec4 skinnedPos = skinMat * vec4(aPos, 1.0);\n"
        "    vec3 skinnedNormal = mat3(skinMat) * aNormal;\n"
        "    FragPos = vec3(model * skinnedPos);\n"
        "    Normal = mat3(transpose(inverse(model))) * skinnedNormal;\n"
        "    TexCoord = aTexCoord;\n"
        "    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
        "}\n";

    std::string frag =
        "#version 460 core\n"
        "out vec4 FragColor;\n"
        "in vec3 FragPos;\n"
        "in vec3 Normal;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D texture1;\n"
        "uniform int hasTexture;\n"
        "uniform vec3 tintColor;\n"
        "uniform vec3 lightDir;\n"
        "uniform vec3 materialDiffuse;\n"
        "uniform float opacity;\n"
        "void main() {\n"
        "    vec3 baseColor;\n"
        "    float alpha = 1.0;\n"
        "    if (hasTexture == 1) {\n"
        "        vec4 texColor = texture(texture1, TexCoord);\n"
        "        if (texColor.a < 0.01) discard;\n"
        "        baseColor = texColor.rgb;\n"
        "        alpha = texColor.a;\n"
        "    } else {\n"
        "        baseColor = materialDiffuse;\n"
        "    }\n"
        "    vec3 norm = normalize(Normal);\n"
        "    vec3 light = normalize(-lightDir);\n"
        "    float diff = max(dot(norm, light), 0.0);\n"
        "    vec3 ambient = 0.3 * baseColor;\n"
        "    vec3 diffuse = diff * baseColor;\n"
        "    FragColor = vec4(ambient + diffuse, alpha * opacity);\n"
        "}\n";

    return LoadFromSource(vert.c_str(), frag.c_str());
}

bool Shader::CreateWater()
{
    std::string vert = std::string(shaderHeader) +
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoord;\n"
        "out VS_OUT {\n"
        "    vec3 pos;\n"
        "    vec3 normal;\n"
        "    vec2 texCoord;\n"
        "} vs_out;\n"
        "void main() {\n"
        "    vs_out.pos = aPos;\n"
        "    vs_out.normal = aNormal;\n"
        "    vs_out.texCoord = aTexCoord;\n"
        "}\n";

    std::string geom = std::string(shaderHeader) +
        "layout(triangles) in;\n"
        "layout(triangle_strip, max_vertices = 12) out;\n"
        "in VS_OUT {\n"
        "    vec3 pos;\n"
        "    vec3 normal;\n"
        "    vec2 texCoord;\n"
        "} gs_in[];\n"
        "out vec3 FragPos;\n"
        "out vec3 Normal;\n"
        "out vec2 TexCoord;\n"
        "out float v_Height;\n"
        "uniform mat4 model;\n"
        "uniform float u_Time;\n"
        "uniform float waveSpeed;\n"
        "uniform float waveAmplitude;\n"
        "uniform float waveFrequency;\n"
        "struct Wave { vec2 direction; float steepness; float wavelength; };\n"
        "vec3 GerstnerWave(vec3 p, Wave w, float time, inout vec3 tangent, inout vec3 binormal) {\n"
        "    float k = 2.0 * 3.14159 / w.wavelength;\n"
        "    float c = sqrt(9.8 / k);\n"
        "    vec2 d = normalize(w.direction);\n"
        "    float f = k * (dot(d, p.xz) - c * time * waveSpeed);\n"
        "    float a = (w.steepness / k) * waveAmplitude;\n"
        "    float overlapCheck = w.steepness * waveAmplitude;\n"
        "    float hScale = (overlapCheck > 0.35) ? 0.35 / overlapCheck : 1.0;\n"
        "    float cosf = cos(f); float sinf = sin(f);\n"
        "    vec3 disp = vec3(d.x*(a*hScale*cosf), a*sinf, d.y*(a*hScale*cosf));\n"
        "    float wa = k*a; float wa_h = wa*hScale;\n"
        "    tangent  += vec3(-d.x*d.x*(wa_h*sinf), d.x*(wa*cosf), -d.x*d.y*(wa_h*sinf));\n"
        "    binormal += vec3(-d.x*d.y*(wa_h*sinf), d.y*(wa*cosf), -d.y*d.y*(wa_h*sinf));\n"
        "    return disp;\n"
        "}\n"
        "vec3 ApplyWaves(vec3 p, out vec3 normal, out float height) {\n"
        "    vec3 fp = p; vec3 t = vec3(1,0,0); vec3 b = vec3(0,0,1);\n"
        "    Wave w1 = Wave(vec2(1.0,0.5),  0.15, 6.0/waveFrequency);\n"
        "    Wave w2 = Wave(vec2(0.6,1.0),  0.10, 3.5/waveFrequency);\n"
        "    Wave w3 = Wave(vec2(-0.4,0.8), 0.10, 2.0/waveFrequency);\n"
        "    fp += GerstnerWave(p,w1,u_Time,t,b);\n"
        "    fp += GerstnerWave(p,w2,u_Time,t,b);\n"
        "    fp += GerstnerWave(p,w3,u_Time,t,b);\n"
        "    normal = normalize(cross(b,t)); height = fp.y;\n"
        "    return fp;\n"
        "}\n"
        "void EmitV(vec3 pos, vec2 tex) {\n"
        "    vec3 n; float h;\n"
        "    vec3 wp = ApplyWaves(pos, n, h);\n"
        "    v_Height = h;\n"
        "    FragPos = vec3(model * vec4(wp, 1.0));\n"
        "    Normal = normalize(mat3(transpose(inverse(model))) * n);\n"
        "    TexCoord = tex;\n"
        "    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
        "    EmitVertex();\n"
        "}\n"
        "void main() {\n"
        "    vec3 v0=gs_in[0].pos, v1=gs_in[1].pos, v2=gs_in[2].pos;\n"
        "    vec3 m1=(v0+v1)/2.0, m2=(v1+v2)/2.0, m3=(v2+v0)/2.0;\n"
        "    vec2 t0=gs_in[0].texCoord, t1=gs_in[1].texCoord, t2=gs_in[2].texCoord;\n"
        "    vec2 tm1=(t0+t1)/2.0, tm2=(t1+t2)/2.0, tm3=(t2+t0)/2.0;\n"
        "    EmitV(v0,t0);   EmitV(m1,tm1); EmitV(m3,tm3); EndPrimitive();\n"
        "    EmitV(m3,tm3);  EmitV(m2,tm2); EmitV(v2,t2);  EndPrimitive();\n"
        "    EmitV(m1,tm1);  EmitV(v1,t1);  EmitV(m2,tm2); EndPrimitive();\n"
        "    EmitV(m1,tm1);  EmitV(m2,tm2); EmitV(m3,tm3); EndPrimitive();\n"
        "}\n";

    std::string frag =
        "#version 460 core\n"
        "out vec4 FragColor;\n"
        "in vec3 FragPos;\n"
        "in vec3 Normal;\n"
        "in vec2 TexCoord;\n"
        "in float v_Height;\n"
        "uniform vec3 lightDir;\n"
        "uniform vec3 viewPos;\n"
        "uniform float waveAmplitude;\n"
        "uniform float opacity;\n"
        "void main() {\n"
        "    vec3 viewDir = normalize(viewPos - FragPos);\n"
        "    vec3 norm = normalize(Normal);\n"
        "    vec3 light = normalize(-lightDir);\n"
        "    vec3 halfway = normalize(light + viewDir);\n"
        "    vec3 deep    = vec3(0.0, 0.1, 0.4);\n"
        "    vec3 shallow = vec3(0.0, 0.4, 0.6);\n"
        "    vec3 foam    = vec3(1.0, 1.0, 1.0);\n"
        "    float hf = smoothstep(-0.5*waveAmplitude, 0.8*waveAmplitude, v_Height);\n"
        "    vec3 albedo = mix(deep, shallow, hf);\n"
        "    float ff = smoothstep(0.4*waveAmplitude, 0.6*waveAmplitude, v_Height);\n"
        "    albedo = mix(albedo, foam, ff);\n"
        "    vec3 ambient = 0.1 * albedo;\n"
        "    float diff = max(dot(norm, light), 0.0);\n"
        "    float spec = pow(max(dot(norm, halfway), 0.0), 128.0);\n"
        "    float F0 = 0.02;\n"
        "    float fresnel = F0 + (1.0-F0) * pow(1.0 - max(dot(norm,viewDir),0.0), 5.0);\n"
        "    vec3 result = ambient + diff*albedo + vec3(0.8)*spec;\n"
        "    float alpha = mix(0.7, 0.95, fresnel);\n"
        "    FragColor = vec4(result, alpha * opacity);\n"
        "}\n";

    return LoadFromSource(vert.c_str(), frag.c_str(), geom.c_str());
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