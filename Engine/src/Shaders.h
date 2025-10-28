#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader();
    ~Shader();

    bool Create();
    void Use() const;
    void Delete();

    unsigned int GetProgramID() const { return shaderProgram; }

    // Utility functions for setting uniforms
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;

private:
    unsigned int shaderProgram;
};