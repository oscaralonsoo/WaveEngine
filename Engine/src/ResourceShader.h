#pragma once

#include "ModuleResources.h"
#include "Shaders.h"

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

class ResourceShader : public Resource
{
public:
    enum class UniformType { Float, Int, Bool, Vec3, Vec4 };

    struct UniformInfo
    {
        std::string name;
        UniformType type;
    };

public:
    ResourceShader(UID uid);
    ~ResourceShader() override;

    bool LoadInMemory() override;
    void UnloadFromMemory() override;

    void SetVertexPath(const std::string& path) { vertexPath = path; }
    void SetFragmentPath(const std::string& path) { fragmentPath = path; }

    const std::string& GetVertexPath() const { return vertexPath; }
    const std::string& GetFragmentPath() const { return fragmentPath; }

    Shader* GetRuntimeShader() { return runtimeShader.get(); }
    const std::vector<UniformInfo>& GetUniforms() const { return uniforms; }
    const std::string& GetLastError() const { return lastError; }

    bool CheckAndHotReload();

private:
    bool CompileFromFiles(std::string& outError);
    void ParseUniformsFromSources(const std::string& vs, const std::string& fs);
    static bool ReadTextFile(const std::string& path, std::string& outText);

private:
    std::string vertexPath;
    std::string fragmentPath;

    std::unique_ptr<Shader> runtimeShader;
    std::vector<UniformInfo> uniforms;
    std::string lastError;

    std::uint64_t vsStamp = 0;
    std::uint64_t fsStamp = 0;
};
