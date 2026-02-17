#pragma once
#include "Module.h"
#include "FileSystem.h"
#include "Shader.h"
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
    };
};

class Renderer : public Module
{
    struct RenderLine {
        glm::vec3 start;
        glm::vec3 end;
        glm::vec4 color;
    };

public:
    Renderer();
    ~Renderer();

    bool Start() override;
    bool Update() override;
    bool CleanUp() override;
    bool PreUpdate() override;

    Texture* GetDefaultTexture() const { return defaultTexture.get(); }

    // Mesh management
    void LoadMesh(Mesh& mesh);
    void DrawMesh(const Mesh& mesh);
    void UnloadMesh(Mesh& mesh);
    void LoadTexture(const std::string& path);

    // Scene rendering
    void DrawScene();
    void DrawScene(ComponentCamera* renderCamera, ComponentCamera* cullingCamera, bool drawEditorFeatures = true);

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
    Shader* GetWaterShader() const { return waterShader.get(); }
    Shader* GetOutlineShader() const { return outlineShader.get(); }
    Shader* GetLineShader() const { return lineShader.get(); }

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

    void SetLightDir(const glm::vec3& dir) { lightDir = dir; }
    glm::vec3 GetLightDir() const { return lightDir; }

	// Framebuffer management (Scene window)
    void CreateFramebuffer(int width, int height);
    void ResizeFramebuffer(int width, int height);
    void BindFramebuffer();
    void UnbindFramebuffer();
    GLuint GetSceneTexture() const { return sceneTexture; }

    // Framebuffer management (Game window)
    void CreateGameFramebuffer(int width, int height);
    void ResizeGameFramebuffer(int width, int height);
    void BindGameFramebuffer();
    GLuint GetGameTexture() const { return gameTexture; }

    // zBuffer visualization
    bool IsShowingZBuffer() const { return showZBuffer; }
    void SetShowZBuffer(bool show) { showZBuffer = show; }

    // Draw forms
    void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
    void DrawArc(glm::vec3 center, glm::quat rotation, float r, int segments, glm::vec4 col, glm::vec3 axisA, glm::vec3 axisB);
    void DrawCircle(glm::vec3 center, glm::quat rotation, float r, int segments, glm::vec4 col, glm::vec3 axisA, glm::vec3 axisB);
    void DrawSphere(const glm::vec3& center, float radius, const glm::vec4& color, int segments = 16);

private:
    // Internal rendering methods
    void DrawGameObjectIterative(GameObject* gameObject,
        bool renderTransparentOnly,
        ComponentCamera* renderCamera,
        ComponentCamera* cullingCamera);
    void DrawGameObjectWithStencil(GameObject* gameObject);
    void DrawLinesList(const ComponentCamera* camera);
    void ApplyRenderSettings();

    //Checkers
    bool IsGameObjectAndParentsActive(GameObject* gameObject) const;

    // Shaders
    std::unique_ptr<Shader> defaultShader;
    std::unique_ptr<Shader> waterShader;
    std::unique_ptr<Shader> lineShader;
    std::unique_ptr<Shader> outlineShader;
    std::unique_ptr<Shader> depthShader;

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
    glm::vec3 lightDir = glm::vec3(1.0f, -1.0f, -1.0f);

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

    // Framebuffer (Scene window)
    GLuint fbo = 0;
    GLuint sceneTexture = 0;
    GLuint rbo = 0;
    int framebufferWidth = 1280;
    int framebufferHeight = 720;

    // Framebuffer (Game window)
    GLuint gameFbo = 0;
    GLuint gameTexture = 0;
    GLuint gameRbo = 0;
    int gameFramebufferWidth = 1280;
    int gameFramebufferHeight = 720;

    // zBuffer visualization
    bool showZBuffer = false;

    void DrawRay(const glm::vec3& origin, const glm::vec3& direction,
        float length, const glm::vec3& color);
    void DrawAABB(const glm::vec3& min, const glm::vec3& max,
        const glm::vec3& color);
    void DrawAllAABBs(GameObject* gameObject);

    std::vector<RenderLine> linesList;
};