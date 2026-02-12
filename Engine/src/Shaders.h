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

    void Use() const;
    void Delete();

    unsigned int GetProgramID() const { return shaderProgram; }

    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;
    void SetInt(const std::string& name, int value) const; // NUEVO

    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetBool(const std::string& name, bool value) const;

    // NEW: compile from external sources / files
    bool CreateFromSource(const std::string& vertexSrc,
        const std::string& fragmentSrc,
        std::string* outErrorLog = nullptr);


private:
    unsigned int shaderProgram;
};