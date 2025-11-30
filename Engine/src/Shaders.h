#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader();
    ~Shader();

    // Shader creation methods for different use cases
    bool Create(); // Standard textured shader
    bool CreateSimpleColor(); // Color shader for transparent objects
    bool CreateWithDiscard(); // Shader with alpha discard for cutout textures
    bool CreateSingleColor(); // Solid color shader for outlines
    bool CreateDepthVisualization(); // Z-Buffer visualization shader

    void Use() const;
    void Delete();

    unsigned int GetProgramID() const { return shaderProgram; }

    // Uniform setters
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;

private:
    unsigned int shaderProgram;
};