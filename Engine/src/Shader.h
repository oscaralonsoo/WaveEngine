#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader();
    ~Shader();

    bool Create();
    bool CreateSimpleColor();
    bool CreateWithDiscard();
    bool CreateSingleColor();
    bool CreateDepthVisualization();
    bool CreateNoTexture(); 
    bool CreateWater();

    bool LoadFromSource(const char* vSource, const char* fSource, const char* gSource = nullptr);

    void Use() const;
    void Delete();

    unsigned int GetProgramID() const { return shaderProgram; }

    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;
    void SetInt(const std::string& name, int value) const;

    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetBool(const std::string& name, bool value) const;

private:
    unsigned int CompileShader(unsigned int type, const char* source);

    unsigned int shaderProgram;
};