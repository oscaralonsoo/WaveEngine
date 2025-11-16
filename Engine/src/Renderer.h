#pragma once
#include "Module.h"
#include "FileSystem.h"
#include "Shaders.h"
#include "Texture.h"
#include "Frustum.h"
#include <memory>
#include "Primitives.h"
#include "ComponentCamera.h"

class GameObject;

// Transparent objects must be sorted and rendered back-to-front
struct TransparentObject
{
    GameObject* gameObject;
    float distanceToCamera;

    TransparentObject(GameObject* obj, float dist)
        : gameObject(obj), distanceToCamera(dist) {
    }
};

class Renderer : public Module
{
public:
    Renderer();
    ~Renderer();

    bool Start() override;
    bool Update() override;
    bool CleanUp() override;
    bool PreUpdate() override;

    // Mesh management
    void LoadMesh(Mesh& mesh);
    void DrawMesh(const Mesh& mesh);
    void UnloadMesh(Mesh& mesh);
    void LoadTexture(const std::string& path);

    // Scene rendering
    void DrawScene();

    // Transparency handling
    bool HasTransparency(GameObject* gameObject);
    void CollectTransparentObjects(GameObject* gameObject,
        std::vector<TransparentObject>& transparentObjects);

    // Frustum culling
    bool ShouldCullGameObject(GameObject* gameObject, const Frustum& frustum);

    // Debug visualization
    void DrawVertexNormals(const Mesh& mesh, const glm::mat4& modelMatrix);
    void DrawFaceNormals(const Mesh& mesh, const glm::mat4& modelMatrix);

    // Shader access
    Shader* GetDefaultShader() const { return defaultShader.get(); }
    Shader* GetOutlineShader() const { return outlineShader.get(); }

    ComponentCamera* GetCamera();

    void DrawCameraFrustum(ComponentCamera* camera, const glm::vec3& color);

    // Render configuration
    bool IsDepthTestEnabled() const { return depthTestEnabled; }
    void SetDepthTest(bool enabled);

    bool IsFaceCullingEnabled() const { return faceCullingEnabled; }
    void SetFaceCulling(bool enabled);

    bool IsWireframeMode() const { return wireframeMode; }
    void SetWireframeMode(bool enabled);

    void GetClearColor(float& r, float& g, float& b) const { r = clearColorR; g = clearColorG; b = clearColorB; }
    void SetClearColor(float r, float g, float b);

    int GetCullFaceMode() const { return cullFaceMode; }
    void SetCullFaceMode(int mode); // 0=Back, 1=Front, 2=Both

private:
    // Internal rendering methods
    void DrawGameObjectRecursive(GameObject* gameObject,
        bool renderTransparentOnly,
        ComponentCamera* renderCamera,
        ComponentCamera* cullingCamera);
    void DrawGameObjectWithStencil(GameObject* gameObject);
    void ApplyRenderSettings();

    // Shaders
    std::unique_ptr<Shader> defaultShader;
    std::unique_ptr<Shader> lineShader;
    std::unique_ptr<Shader> outlineShader;

    // Default assets
    std::unique_ptr<Texture> defaultTexture;
    Mesh sphere, cube, pyramid, cylinder, plane;

    // OpenGL state
    bool depthTestEnabled = true;
    bool faceCullingEnabled = true;
    bool wireframeMode = false;
    float clearColorR = 0.2f;
    float clearColorG = 0.25f;
    float clearColorB = 0.3f;
    int cullFaceMode = 0; // GL_BACK

    // Normal visualization buffers (reused to avoid repeated allocations)
    GLuint normalLinesVAO = 0;
    GLuint normalLinesVBO = 0;
    size_t normalLinesCapacity = 0;

    // Cached uniform locations to avoid repeated lookups
    struct ShaderUniforms {
        GLint projection = -1;
        GLint view = -1;
        GLint model = -1;
        GLint texture1 = -1;
    } defaultUniforms, lineUniforms, outlineUniforms;
};