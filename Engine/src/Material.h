#pragma once

#include <string>

class Shader;

enum MaterialType
{
    STANDARD,
    UNKNOWN
};

class Material
{
public:
    
    Material(MaterialType type) : type(type) {}
    virtual ~Material() = default;

    virtual void Bind(Shader* shader) = 0;

    MaterialType GetType() const { return type; }
    const std::string& GetName() const { return name; }
    void SetName(const std::string& newName) { name = newName; }

private:
    
    MaterialType type;
    std::string name;
};