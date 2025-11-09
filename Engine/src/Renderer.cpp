#include "Renderer.h"
#include "Application.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ModuleEditor.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer()
{
    LOG_DEBUG("Renderer Constructor");
    camera = make_unique<Camera>();
}

Renderer::~Renderer()
{
}

bool Renderer::Start()
{
    LOG_DEBUG("=== Initializing Renderer Module ===");
    LOG_CONSOLE("Initializing renderer and shaders...");

    // Enable required OpenGL features for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize default shader
    defaultShader = make_unique<Shader>();

    if (!defaultShader->Create())
    {
        LOG_DEBUG("ERROR: Failed to create default shader");
        LOG_CONSOLE("ERROR: Failed to compile shaders");
        return false;
    }
    else
    {
        LOG_DEBUG("Shader created successfully - Program ID: %d", defaultShader->GetProgramID());
        LOG_CONSOLE("OpenGL shaders compiled successfully");
    }

    // Initialize line shader for debug visualization
    lineShader = make_unique<Shader>();

    if (!lineShader->CreateSimpleColor())
    {
        LOG_DEBUG("ERROR: Failed to create line shader");
        LOG_CONSOLE("ERROR: Failed to compile line shader");
        return false;
    }
    else
    {
        LOG_DEBUG("Line shader created successfully - Program ID: %d", lineShader->GetProgramID());
        LOG_CONSOLE("Line shader compiled successfully");
    }

    // Initialize outline shader for selection highlighting
    outlineShader = make_unique<Shader>();

    if (!outlineShader->CreateSingleColor())
    {
        LOG_DEBUG("ERROR: Failed to create outline shader");
        LOG_CONSOLE("ERROR: Failed to compile outline shader");
        return false;
    }
    else
    {
        LOG_DEBUG("Outline shader created successfully - Program ID: %d", outlineShader->GetProgramID());
        LOG_CONSOLE("Outline shader compiled successfully");
    }

    // Generate default checkerboard texture for untextured objects
    defaultTexture = make_unique<Texture>();
    defaultTexture->CreateCheckerboard();
    LOG_DEBUG("Default checkerboard texture created");

    LOG_DEBUG("Renderer initialized successfully");
    LOG_CONSOLE("Renderer ready");

    // Cache uniform locations to avoid repeated string lookups
    defaultUniforms.projection = glGetUniformLocation(defaultShader->GetProgramID(), "projection");
    defaultUniforms.view = glGetUniformLocation(defaultShader->GetProgramID(), "view");
    defaultUniforms.model = glGetUniformLocation(defaultShader->GetProgramID(), "model");
    defaultUniforms.texture1 = glGetUniformLocation(defaultShader->GetProgramID(), "texture1");

    outlineUniforms.projection = glGetUniformLocation(outlineShader->GetProgramID(), "projection");
    outlineUniforms.view = glGetUniformLocation(outlineShader->GetProgramID(), "view");
    outlineUniforms.model = glGetUniformLocation(outlineShader->GetProgramID(), "model");

    return true;
}

void Renderer::LoadMesh(Mesh& mesh)
{
    // Create and configure VAO
    glGenVertexArrays(1, &mesh.VAO);
    glBindVertexArray(mesh.VAO);

    // Upload vertex data to GPU
    glGenBuffers(1, &mesh.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), &mesh.vertices[0], GL_STATIC_DRAW);

    // Configure vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    // Upload index data
    glGenBuffers(1, &mesh.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), &mesh.indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    LOG_DEBUG("Mesh loaded - VAO: %d, Vertices: %d, Indices: %d", mesh.VAO, mesh.vertices.size(), mesh.indices.size());
}

void Renderer::DrawMesh(const Mesh& mesh)
{
    if (mesh.VAO == 0)
    {
        LOG_DEBUG("ERROR: Trying to draw mesh without VAO");
        LOG_CONSOLE("Render error: Invalid mesh");
        return;
    }

    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Renderer::UnloadMesh(Mesh& mesh)
{
    if (mesh.VAO != 0)
    {
        glDeleteVertexArrays(1, &mesh.VAO);
        mesh.VAO = 0;
    }

    if (mesh.VBO != 0)
    {
        glDeleteBuffers(1, &mesh.VBO);
        mesh.VBO = 0;
    }

    if (mesh.EBO != 0)
    {
        glDeleteBuffers(1, &mesh.EBO);
        mesh.EBO = 0;
    }
}

void Renderer::LoadTexture(const std::string& path)
{
    LOG_DEBUG("Renderer: Loading new texture");
    LOG_CONSOLE("Loading texture...");

    auto newTexture = make_unique<Texture>();

    if (newTexture->LoadFromFile(path))
    {
        defaultTexture = std::move(newTexture);
        LOG_DEBUG("Renderer: Texture applied successfully");
        LOG_CONSOLE("Texture applied to scene");
    }
    else
    {
        LOG_DEBUG("ERROR: Renderer failed to load texture: %s", path.c_str());
        LOG_CONSOLE("Failed to apply texture");
    }
}

bool Renderer::PreUpdate()
{
    return true;
}

bool Renderer::Update()
{
    // Clear buffers
    glClearColor(clearColorR, clearColorG, clearColorB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    defaultShader->Use();

    camera->Update();

    // Update projection matrix with current aspect ratio
    int width, height;
    Application::GetInstance().window->GetWindowSize(width, height);

    float aspectRatio = (float)width / (float)height;
    camera->SetAspectRatio(aspectRatio);

    GLuint shaderProgram = defaultShader->GetProgramID();

    // Update camera matrices
    glUniformMatrix4fv(defaultUniforms.projection, 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(defaultUniforms.view, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));

    // Bind default texture
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(defaultUniforms.texture1, 0);

    GameObject* root = Application::GetInstance().scene->GetRoot();

    if (root != nullptr && root->GetChildren().size() > 0)
    {
        DrawScene();
    }

    defaultTexture->Unbind();

    return true;
}

bool Renderer::CleanUp()
{
    LOG_DEBUG("Cleaning up Renderer");

    // Release primitive meshes
    UnloadMesh(sphere);
    UnloadMesh(cylinder);
    UnloadMesh(pyramid);

    if (defaultShader)
    {
        defaultShader->Delete();
    }

    if (lineShader)
    {
        lineShader->Delete();
    }

    if (outlineShader)
    {
        outlineShader->Delete();
    }

    if (normalLinesVAO != 0)
    {
        glDeleteVertexArrays(1, &normalLinesVAO);
        glDeleteBuffers(1, &normalLinesVBO);
    }

    LOG_DEBUG("Renderer cleaned up successfully");
    LOG_CONSOLE("Renderer shutdown complete");

    return true;
}

bool Renderer::HasTransparency(GameObject* gameObject)
{
    ComponentMaterial* material = static_cast<ComponentMaterial*>(
        gameObject->GetComponent(ComponentType::MATERIAL));

    if (material && material->IsActive())
    {
        return true;
    }

    return false;
}

void Renderer::CollectTransparentObjects(GameObject* gameObject,
    std::vector<TransparentObject>& transparentObjects)
{
    if (!gameObject->IsActive())
        return;

    Transform* transform = static_cast<Transform*>(
        gameObject->GetComponent(ComponentType::TRANSFORM));

    if (transform != nullptr && HasTransparency(gameObject))
    {
        glm::vec3 objectPos = glm::vec3(transform->GetGlobalMatrix()[3]);
        float distance = glm::length(camera->GetPosition() - objectPos);

        transparentObjects.emplace_back(gameObject, distance);
    }

    for (GameObject* child : gameObject->GetChildren())
    {
        CollectTransparentObjects(child, transparentObjects);
    }
}

void Renderer::DrawScene()
{
    GameObject* root = Application::GetInstance().scene->GetRoot();
    if (root == nullptr)
        return;

    SelectionManager* selectionMgr = Application::GetInstance().selectionManager;
    const std::vector<GameObject*>& selectedObjects = selectionMgr->GetSelectedObjects();

    // First pass: render all opaque objects
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilMask(0x00);
    DrawGameObjectRecursive(root, false);

    // Second pass: render selection outlines
    outlineShader->Use();
    glUniformMatrix4fv(outlineUniforms.projection, 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(outlineUniforms.view, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
    outlineShader->SetVec3("outlineColor", glm::vec3(1.0f, 0.41f, 0.71f));

    float outlineScale = 1.02f;

    // Disable depth test so outlines render on top
    glDisable(GL_DEPTH_TEST);

    for (GameObject* selectedObj : selectedObjects)
    {
        Transform* transform = static_cast<Transform*>(selectedObj->GetComponent(ComponentType::TRANSFORM));
        if (transform == nullptr) continue;

        glm::mat4 modelMatrix = transform->GetGlobalMatrix();

        // Decompose matrix into position, rotation, and scale
        glm::vec3 position = glm::vec3(modelMatrix[3]);

        glm::vec3 scale;
        scale.x = glm::length(glm::vec3(modelMatrix[0]));
        scale.y = glm::length(glm::vec3(modelMatrix[1]));
        scale.z = glm::length(glm::vec3(modelMatrix[2]));

        // Extract rotation by normalizing basis vectors
        glm::mat4 rotationMatrix = modelMatrix;
        rotationMatrix[0] /= scale.x;
        rotationMatrix[1] /= scale.y;
        rotationMatrix[2] /= scale.z;
        rotationMatrix[3] = glm::vec4(0, 0, 0, 1);

        // Apply outline scaling
        glm::vec3 scaledScale = scale * outlineScale;

        // Reconstruct matrix with scaled version
        glm::mat4 outlineModelMatrix = glm::translate(glm::mat4(1.0f), position) *
            rotationMatrix *
            glm::scale(glm::mat4(1.0f), scaledScale);

        glUniformMatrix4fv(outlineUniforms.model, 1, GL_FALSE, glm::value_ptr(outlineModelMatrix));

        const std::vector<Component*>& meshComponents =
            selectedObj->GetComponentsOfType(ComponentType::MESH);

        for (Component* comp : meshComponents)
        {
            ComponentMesh* meshComp = static_cast<ComponentMesh*>(comp);

            if (meshComp->IsActive() && meshComp->HasMesh())
            {
                const Mesh& mesh = meshComp->GetMesh();
                DrawMesh(mesh);
            }
        }
    }

    // Restore state
    glEnable(GL_DEPTH_TEST);
    defaultShader->Use();

    // Third pass: render transparent objects back-to-front
    std::vector<TransparentObject> transparentObjects;
    CollectTransparentObjects(root, transparentObjects);

    std::sort(transparentObjects.begin(), transparentObjects.end(),
        [](const TransparentObject& a, const TransparentObject& b) {
            return a.distanceToCamera > b.distanceToCamera;
        });

    for (const auto& transparentObj : transparentObjects)
    {
        DrawGameObjectRecursive(transparentObj.gameObject, true);
    }
}

void Renderer::DrawGameObjectWithStencil(GameObject* gameObject)
{
    if (!gameObject->IsActive())
        return;

    Transform* transform = static_cast<Transform*>(gameObject->GetComponent(ComponentType::TRANSFORM));
    if (transform == nullptr) return;

    const glm::mat4& modelMatrix = transform->GetGlobalMatrix();
    glUniformMatrix4fv(defaultUniforms.model, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    defaultShader->SetVec3("tintColor", glm::vec3(1.0f));

    ComponentMaterial* material = static_cast<ComponentMaterial*>(
        gameObject->GetComponent(ComponentType::MATERIAL));

    bool materialBound = false;
    if (material && material->IsActive())
    {
        material->Use();
        materialBound = true;
    }
    else
    {
        defaultTexture->Bind();
    }

    const std::vector<Component*>& meshComponents =
        gameObject->GetComponentsOfType(ComponentType::MESH);

    for (Component* comp : meshComponents)
    {
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(comp);

        if (meshComp->IsActive() && meshComp->HasMesh())
        {
            const Mesh& mesh = meshComp->GetMesh();
            DrawMesh(mesh);
        }
    }

    if (materialBound)
        material->Unbind();
    else
        defaultTexture->Unbind();
}

void Renderer::DrawGameObjectRecursive(GameObject* gameObject, bool renderTransparentOnly)
{
    if (!gameObject->IsActive())
        return;

    Transform* transform = static_cast<Transform*>(gameObject->GetComponent(ComponentType::TRANSFORM));
    if (transform == nullptr) return;

    bool isTransparent = HasTransparency(gameObject);

    // Skip this object if transparency doesn't match the current pass
    if (renderTransparentOnly != isTransparent)
    {
        for (GameObject* child : gameObject->GetChildren())
        {
            DrawGameObjectRecursive(child, renderTransparentOnly);
        }
        return;
    }

    const glm::mat4& modelMatrix = transform->GetGlobalMatrix();
    glUniformMatrix4fv(defaultUniforms.model, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    defaultShader->SetVec3("tintColor", glm::vec3(1.0f));

    ComponentMaterial* material = static_cast<ComponentMaterial*>(
        gameObject->GetComponent(ComponentType::MATERIAL));

    bool materialBound = false;
    if (material && material->IsActive())
    {
        material->Use();
        materialBound = true;
    }
    else
    {
        defaultTexture->Bind();
    }

    const std::vector<Component*>& meshComponents =
        gameObject->GetComponentsOfType(ComponentType::MESH);

    ModuleEditor* editor = Application::GetInstance().editor.get();
    SelectionManager* selectionMgr = Application::GetInstance().selectionManager;

    // Check if normals should be displayed for this object
    bool shouldDrawNormals = false;
    if (editor && selectionMgr->IsSelected(gameObject))
    {
        shouldDrawNormals = true;
    }
    else if (editor)
    {
        // Check parent hierarchy for selection
        GameObject* parent = gameObject->GetParent();
        while (parent)
        {
            if (selectionMgr->IsSelected(parent))
            {
                shouldDrawNormals = true;
                break;
            }
            parent = parent->GetParent();
        }
    }

    const bool showVertex = editor && editor->ShouldShowVertexNormals();
    const bool showFace = editor && editor->ShouldShowFaceNormals();

    for (Component* comp : meshComponents)
    {
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(comp);

        if (meshComp->IsActive() && meshComp->HasMesh())
        {
            const Mesh& mesh = meshComp->GetMesh();
            DrawMesh(mesh);

            if (shouldDrawNormals)
            {
                if (showVertex) DrawVertexNormals(mesh, modelMatrix);
                if (showFace) DrawFaceNormals(mesh, modelMatrix);
            }
        }
    }

    if (materialBound)
        material->Unbind();
    else
        defaultTexture->Unbind();

    // Recursively render children
    for (GameObject* child : gameObject->GetChildren())
    {
        DrawGameObjectRecursive(child, renderTransparentOnly);
    }
}

void Renderer::DrawVertexNormals(const Mesh& mesh, const glm::mat4& modelMatrix)
{
    if (!mesh.IsValid() || mesh.vertices.empty())
        return;

    std::vector<float> lineVertices;
    lineVertices.reserve(mesh.vertices.size() * 6);

    const float normalLength = 0.2f;
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));

    // Generate line endpoints for each vertex normal
    for (const auto& vertex : mesh.vertices)
    {
        glm::vec4 worldPos = modelMatrix * glm::vec4(vertex.position, 1.0f);
        glm::vec3 worldNormal = glm::normalize(normalMatrix * vertex.normal);
        glm::vec3 endPoint = glm::vec3(worldPos) + worldNormal * normalLength;

        lineVertices.insert(lineVertices.end(), {
            worldPos.x, worldPos.y, worldPos.z,
            endPoint.x, endPoint.y, endPoint.z
            });
    }

    // Create or reuse VAO/VBO
    if (normalLinesVAO == 0)
    {
        glGenVertexArrays(1, &normalLinesVAO);
        glGenBuffers(1, &normalLinesVBO);

        glBindVertexArray(normalLinesVAO);
        glBindBuffer(GL_ARRAY_BUFFER, normalLinesVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    }
    else
    {
        glBindVertexArray(normalLinesVAO);
        glBindBuffer(GL_ARRAY_BUFFER, normalLinesVBO);
    }

    // Upload data (reallocate only if needed)
    size_t requiredSize = lineVertices.size() * sizeof(float);
    if (requiredSize > normalLinesCapacity)
    {
        glBufferData(GL_ARRAY_BUFFER, requiredSize, lineVertices.data(), GL_DYNAMIC_DRAW);
        normalLinesCapacity = requiredSize;
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, requiredSize, lineVertices.data());
    }

    // Render normals
    lineShader->Use();
    glUniformMatrix4fv(glGetUniformLocation(lineShader->GetProgramID(), "projection"),
        1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(lineShader->GetProgramID(), "view"),
        1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(lineShader->GetProgramID(), "model"),
        1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    lineShader->SetVec3("color", glm::vec3(0.0f, 0.5f, 1.0f));
    glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);

    glBindVertexArray(0);
    defaultShader->Use();
}

void Renderer::DrawFaceNormals(const Mesh& mesh, const glm::mat4& modelMatrix)
{
    if (!mesh.IsValid() || mesh.vertices.empty() || mesh.indices.empty())
        return;

    std::vector<float> lineVertices;
    float normalLength = 0.3f;

    // Calculate face normals from triangle data
    for (size_t i = 0; i < mesh.indices.size(); i += 3)
    {
        const Vertex& v0 = mesh.vertices[mesh.indices[i]];
        const Vertex& v1 = mesh.vertices[mesh.indices[i + 1]];
        const Vertex& v2 = mesh.vertices[mesh.indices[i + 2]];

        glm::vec4 worldPos0 = modelMatrix * glm::vec4(v0.position, 1.0f);
        glm::vec4 worldPos1 = modelMatrix * glm::vec4(v1.position, 1.0f);
        glm::vec4 worldPos2 = modelMatrix * glm::vec4(v2.position, 1.0f);

        glm::vec3 faceCenter = (glm::vec3(worldPos0) + glm::vec3(worldPos1) + glm::vec3(worldPos2)) / 3.0f;

        glm::vec3 edge1 = glm::vec3(worldPos1) - glm::vec3(worldPos0);
        glm::vec3 edge2 = glm::vec3(worldPos2) - glm::vec3(worldPos0);
        glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

        glm::vec3 endPoint = faceCenter + faceNormal * normalLength;

        lineVertices.push_back(faceCenter.x);
        lineVertices.push_back(faceCenter.y);
        lineVertices.push_back(faceCenter.z);

        lineVertices.push_back(endPoint.x);
        lineVertices.push_back(endPoint.y);
        lineVertices.push_back(endPoint.z);
    }

    // Create temporary buffers (face normals rendered less frequently)
    GLuint lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    lineShader->Use();
    GLuint shaderProgram = lineShader->GetProgramID();

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    lineShader->SetVec3("color", glm::vec3(0.0f, 1.0f, 0.5f));

    glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);

    glBindVertexArray(0);
    glDeleteBuffers(1, &lineVBO);
    glDeleteVertexArrays(1, &lineVAO);

    defaultShader->Use();
}

void Renderer::SetDepthTest(bool enabled)
{
    depthTestEnabled = enabled;
    if (enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    LOG_DEBUG("Depth test %s", enabled ? "enabled" : "disabled");
}

void Renderer::SetFaceCulling(bool enabled)
{
    faceCullingEnabled = enabled;
    if (enabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    LOG_DEBUG("Face culling %s", enabled ? "enabled" : "disabled");
}

void Renderer::SetWireframeMode(bool enabled)
{
    wireframeMode = enabled;
    if (enabled)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    LOG_DEBUG("Wireframe mode %s", enabled ? "enabled" : "disabled");
}

void Renderer::SetClearColor(float r, float g, float b)
{
    clearColorR = r;
    clearColorG = g;
    clearColorB = b;
    LOG_DEBUG("Clear color set to (%.2f, %.2f, %.2f)", r, g, b);
}

void Renderer::SetCullFaceMode(int mode)
{
    cullFaceMode = mode;

    switch (mode)
    {
    case 0: glCullFace(GL_BACK); break;
    case 1: glCullFace(GL_FRONT); break;
    case 2: glCullFace(GL_FRONT_AND_BACK); break;
    default: glCullFace(GL_BACK); break;
    }

    const char* modeStr[] = { "Back", "Front", "Front and Back" };
    LOG_DEBUG("Cull face mode set to: %s", modeStr[mode]);
}

void Renderer::ApplyRenderSettings()
{
    SetDepthTest(depthTestEnabled);
    SetFaceCulling(faceCullingEnabled);
    SetWireframeMode(wireframeMode);
    SetCullFaceMode(cullFaceMode);
}