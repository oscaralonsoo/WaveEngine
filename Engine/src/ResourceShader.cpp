#include "ResourceShader.h"
#include "Log.h"

#include <filesystem>
#include <regex>
#include <algorithm>
#include <fstream>
#include <sstream>


namespace fs = std::filesystem;

static std::uint64_t GetStamp(const std::string& p)
{
    std::error_code ec;
    auto t = fs::last_write_time(fs::path(p), ec);
    if (ec) return 0;
    // convert to a monotonic-ish integer
    return (std::uint64_t)t.time_since_epoch().count();
}

ResourceShader::ResourceShader(UID uid)
    : Resource(uid, Resource::Type::SHADER)
{
}

ResourceShader::~ResourceShader()
{
    if (loadedInMemory) UnloadFromMemory();
}

//void ResourceShader::SetVertexPath(const std::string& path)
//{
//    vertexPath = path;
//    SetAssetFile(vertexPath.c_str()); // keep compatibility with existing patterns
//}
//
//void ResourceShader::SetFragmentPath(const std::string& path)
//{
//    fragmentPath = path;
//}

//bool ResourceShader::ReadTextFile(const std::string& path, std::string& outText, std::string& outErr)
//{
//    std::ifstream f(path, std::ios::in);
//    if (!f.is_open())
//    {
//        outErr = "Cannot open file: " + path;
//        return false;
//    }
//    std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
//    outText = s;
//    return true;
//}
bool ResourceShader::ReadTextFile(const std::string& path, std::string& outText)
{
    std::ifstream f(path, std::ios::in);
    if (!f.is_open()) return false;

    outText.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return true;
}


void ResourceShader::ParseUniformsFromSources(const std::string& vs, const std::string& fsText)
{
    uniforms.clear();

    // Only parse basic types we want to expose in inspector
    // Ignore matrices and samplers by default; model/view/projection are engine-controlled.
    std::regex re(R"(uniform\s+(float|int|bool|vec3|vec4)\s+([A-Za-z_][A-Za-z0-9_]*)\s*;)");
    auto parseOne = [&](const std::string& src)
        {
            auto begin = std::sregex_iterator(src.begin(), src.end(), re);
            auto end = std::sregex_iterator();
            for (auto it = begin; it != end; ++it)
            {
                std::string t = (*it)[1].str();
                std::string n = (*it)[2].str();

                // Filter engine reserved uniforms
                if (n == "model" || n == "view" || n == "projection" || n == "time") continue;

                UniformType ut;
                if (t == "float") ut = UniformType::Float;
                else if (t == "int") ut = UniformType::Int;
                else if (t == "bool") ut = UniformType::Bool;
                else if (t == "vec3") ut = UniformType::Vec3;
                else ut = UniformType::Vec4;

                // avoid duplicates (vs + fs)
                auto found = std::find_if(uniforms.begin(), uniforms.end(),
                    [&](const UniformInfo& u) { return u.name == n; });
                if (found == uniforms.end())
                    uniforms.push_back({ n, ut });
            }
        };

    parseOne(vs);
    parseOne(fsText);
}

bool ResourceShader::CompileFromFiles(std::string& outError)
{
    std::string vs, fsSrc, err;
    if (!ReadTextFile(vertexPath, vs))
    {
        outError = err;
        return false;
    }
    if (!ReadTextFile(fragmentPath, fsSrc))
    {
        outError = err;
        return false;
    }


    // Build runtime shader
    if (!runtimeShader)
        runtimeShader = std::make_unique<Shader>();
    else
        runtimeShader->Delete();

    std::string compileErr;
    if (!runtimeShader->CreateFromSource(vs, fsSrc, &compileErr))
    {
        outError = compileErr;
        return false;
    }

    ParseUniformsFromSources(vs, fsSrc);
    return true;
}

bool ResourceShader::LoadInMemory()
{
    if (vertexPath.empty())
    {
        lastError = "Vertex path is empty";
        return false;
    }

    if (fragmentPath.empty())
    {
        // convention: same name, .frag
        fs::path p(vertexPath);
        fragmentPath = (p.parent_path() / (p.stem().string() + ".frag")).string();
    }

    std::string err;
    if (!CompileFromFiles(err))
    {
        lastError = err;
        LOG_CONSOLE("[ResourceShader] Compile failed (UID %llu): %s", uid, lastError.c_str());
        return false;
    }

    vsStamp = GetStamp(vertexPath);
    fsStamp = GetStamp(fragmentPath);
    loadedInMemory = true;
    lastError.clear();

    LOG_CONSOLE("[ResourceShader] Loaded shader UID %llu", uid);
    return true;
}

void ResourceShader::UnloadFromMemory()
{
    if (runtimeShader)
        runtimeShader->Delete();
    runtimeShader.reset();

    uniforms.clear();
    loadedInMemory = false;
}

bool ResourceShader::CheckAndHotReload()
{
    if (!loadedInMemory) return false;
    if (vertexPath.empty() || fragmentPath.empty()) return false;

    std::uint64_t nvs = GetStamp(vertexPath);
    std::uint64_t nfs = GetStamp(fragmentPath);
    if (nvs == 0 || nfs == 0) return false;

    if (nvs != vsStamp || nfs != fsStamp)
    {
        std::string err;
        if (!CompileFromFiles(err))
        {
            lastError = err;
            LOG_CONSOLE("[ResourceShader] HotReload FAILED (UID %llu): %s", uid, lastError.c_str());
            return true; // changed but failed
        }

        vsStamp = nvs;
        fsStamp = nfs;
        lastError.clear();
        LOG_CONSOLE("[ResourceShader] HotReload OK (UID %llu)", uid);
        return true;
    }
    return false;
}
