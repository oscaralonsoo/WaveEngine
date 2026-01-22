#include "Renderer.h"
#include "Application.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ModuleEditor.h"
#include "ReverbZone.h"
#include "ReverbZone.h"


#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stack>

#include "tracy/Tracy.hpp"

Renderer::Renderer()
{
    LOG_DEBUG("Renderer Constructor");
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

    if (!defaultShader->CreateNoTexture())  
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

    depthShader = make_unique<Shader>();
    if (!depthShader->CreateDepthVisualization())
    {
        LOG_DEBUG("ERROR: Failed to create depth visualization shader");
        LOG_CONSOLE("ERROR: Failed to compile depth shader");
        return false;
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

	// Create framebuffer (Scene window)
    CreateFramebuffer(framebufferWidth, framebufferHeight);

    // Create framebuffer (Game window)
    CreateGameFramebuffer(gameFramebufferWidth, gameFramebufferHeight);

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

    if (newTexture->LoadFromLibraryOrFile(path))
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

    ZoneScopedN("RendererUpdate");

    ModuleEditor* editor = Application::GetInstance().editor.get();
    ImVec2 viewportSize = editor->sceneViewportSize;

    if (viewportSize.x > 0 && viewportSize.y > 0)
    {
        ResizeFramebuffer((int)viewportSize.x, (int)viewportSize.y);
    }

    BindFramebuffer();

    // Clear buffers
    glClearColor(clearColorR, clearColorG, clearColorB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (showZBuffer)
    {
        depthShader->Use();
    }
    else
    {
        defaultShader->Use();
    }

    ComponentCamera* camera = Application::GetInstance().camera->GetActiveCamera();
    if (!camera) return true;

    // Update projection matrix with current aspect ratio
    float aspectRatio = viewportSize.x / viewportSize.y;
    if (viewportSize.x > 0 && viewportSize.y > 0)
    {
        camera->SetAspectRatio(aspectRatio);
    }
    Shader* currentShader = showZBuffer ? depthShader.get() : defaultShader.get();
    GLuint shaderProgram = currentShader->GetProgramID();

    // Update camera matrices
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));

    if (showZBuffer)
    {
        depthShader->SetFloat("nearPlane", camera->GetNearPlane());
        depthShader->SetFloat("farPlane", camera->GetFarPlane());
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(defaultUniforms.texture1, 0);
    }

    GameObject* root = Application::GetInstance().scene->GetRoot();

    if (!showZBuffer && Application::GetInstance().grid)
    {
        Application::GetInstance().grid->Draw();
    }

    if (root != nullptr && root->GetChildren().size() > 0)
    {
        DrawScene();
    }

    if (!showZBuffer)
    {
        defaultTexture->Unbind();
    }

    if (!showZBuffer && editor)
    {
        // Draw AABBs if enabled
        if (editor->ShouldShowAABB())
        {
            DrawAllAABBs(root);
        }

        // Draw Octree if enabled
        if (editor->ShouldShowOctree())
        {
            Octree* octree = Application::GetInstance().scene->GetOctree();
            if (octree)
            {
                octree->DebugDraw();
            }
            else
            {
                LOG_DEBUG(" Octree is NULL!");
            }
        }

        // Draw Raycast if enabled
        if (editor->ShouldShowRaycast())
        {
            ModuleScene* scene = Application::GetInstance().scene.get();
            if (scene && scene->lastRayLength > 0.0f)
            {
                LOG_DEBUG("Drawing raycast: origin(%.2f,%.2f,%.2f) length %.2f",
                    scene->lastRayOrigin.x, scene->lastRayOrigin.y, scene->lastRayOrigin.z,
                    scene->lastRayLength);
                DrawRay(scene->lastRayOrigin,
                    scene->lastRayDirection,
                    scene->lastRayLength,
                    glm::vec3(1.0f, 0.0f, 1.0f));
            }
            else
            {
                LOG_DEBUG("No ray to draw (lastRayLength = %.2f)",
                    scene ? scene->lastRayLength : -1.0f);
            }
        }
    }

    UnbindFramebuffer();

    // Game View //////////////
    ComponentCamera* sceneCamera = Application::GetInstance().camera->GetSceneCamera();
    ImVec2 gameViewportSize = editor->gameViewportSize;

    if (sceneCamera && gameViewportSize.x > 0 && gameViewportSize.y > 0)
    {
        ResizeGameFramebuffer((int)gameViewportSize.x, (int)gameViewportSize.y);

        BindGameFramebuffer();

        glClearColor(clearColorR, clearColorG, clearColorB, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        defaultShader->Use();

        float gameAspectRatio = gameViewportSize.x / gameViewportSize.y;
        sceneCamera->SetAspectRatio(gameAspectRatio);

        // Update camera matrices 
        GLuint gameShaderProgram = defaultShader->GetProgramID();
        glUniformMatrix4fv(glGetUniformLocation(gameShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(sceneCamera->GetProjectionMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(gameShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(sceneCamera->GetViewMatrix()));

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(defaultUniforms.texture1, 0);

            if (root != nullptr && root->GetChildren().size() > 0)
        {
            DrawScene(sceneCamera, sceneCamera, false);
        }

        defaultTexture->Unbind();

        UnbindFramebuffer();
    }

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

    if (depthShader)
    {
        depthShader->Delete();
    }

    if (normalLinesVAO != 0)
    {
        glDeleteVertexArrays(1, &normalLinesVAO);
        glDeleteBuffers(1, &normalLinesVBO);
    }

    // Clean up Scene framebuffer
    if (sceneTexture != 0)
    {
        glDeleteTextures(1, &sceneTexture);
        sceneTexture = 0;
    }

    if (rbo != 0)
    {
        glDeleteRenderbuffers(1, &rbo);
        rbo = 0;
    }

    if (fbo != 0)
    {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }

    // Clean up Game framebuffer
    if (gameTexture != 0)
    {
        glDeleteTextures(1, &gameTexture);
        gameTexture = 0;
    }

    if (gameRbo != 0)
    {
        glDeleteRenderbuffers(1, &gameRbo);
        gameRbo = 0;
    }

    if (gameFbo != 0)
    {
        glDeleteFramebuffers(1, &gameFbo);
        gameFbo = 0;
    }

    LOG_DEBUG("Renderer cleaned up successfully");
    LOG_CONSOLE("Renderer shutdown complete");

    return true;
}

void Renderer::CreateFramebuffer(int width, int height)
{
    framebufferWidth = width;
    framebufferHeight = height;

	glGenFramebuffers(1, &fbo); // Create framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Creating a texture for a framebuffer
    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Attach texture to framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);

    // Renderbuffer objects
    glGenRenderbuffers(1, &rbo); // Create renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); // Used to create depth and stencil buffer
    // Attach renderbuffer to framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);


    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_DEBUG("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
        LOG_CONSOLE("ERROR: Failed to create scene framebuffer");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ResizeFramebuffer(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    if (framebufferWidth == width && framebufferHeight == height)
        return;

    framebufferWidth = width;
    framebufferHeight = height;

    // Delete old framebuffer resources
    if (sceneTexture != 0)
        glDeleteTextures(1, &sceneTexture);
    if (rbo != 0)
        glDeleteRenderbuffers(1, &rbo);
    if (fbo != 0)
        glDeleteFramebuffers(1, &fbo);

    CreateFramebuffer(width, height);

}

void Renderer::BindFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

void Renderer::UnbindFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int width, height;
    Application::GetInstance().window->GetWindowSize(width, height);
    glViewport(0, 0, width, height);
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
        ComponentCamera* camera = Application::GetInstance().camera->GetActiveCamera();
        if (camera)
        {
            glm::vec3 objectPos = glm::vec3(transform->GetGlobalMatrix()[3]);
            float distance = glm::length(camera->GetPosition() - objectPos);

            transparentObjects.emplace_back(gameObject, distance);
        }
    }

    for (GameObject* child : gameObject->GetChildren())
    {
        CollectTransparentObjects(child, transparentObjects);
    }
}
bool Renderer::ShouldCullGameObject(GameObject* gameObject, const Frustum& frustum)
{
    const std::vector<Component*>& meshComponents =
        gameObject->GetComponentsOfType(ComponentType::MESH);

    if (meshComponents.empty())
        return false;

    for (Component* comp : meshComponents)
    {
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(comp);

        if (meshComp->IsActive() && meshComp->HasMesh())
        {
            glm::vec3 worldMin, worldMax;
            meshComp->GetWorldAABB(worldMin, worldMax);

            FrustumTestResult result = frustum.ContainsAABB(worldMin, worldMax);

            if (gameObject->GetName() == "Chimney")
            {
                static int debugCounter = 0;
                if (debugCounter++ % 60 == 0)
                {
                    // Display the 6 planes of the frustum
                    LOG_DEBUG("Frustum planes:");
                    for (int i = 0; i < 6; i++)
                    {
                        const auto& plane = frustum.GetPlane(i);
                        const char* names[] = { "NEAR", "FAR", "LEFT", "RIGHT", "TOP", "BOTTOM" };
                        LOG_DEBUG("  %s: n(%.3f, %.3f, %.3f) d=%.3f",
                            names[i],
                            plane.normal.x, plane.normal.y, plane.normal.z,
                            plane.distance);
                    }

                    // Display camera position
                    ComponentCamera* cam = Application::GetInstance().camera->GetSceneCamera();
                    if (cam)
                    {
                        glm::vec3 camPos = cam->GetPosition();
                        glm::vec3 camFront = cam->GetFront();
                        LOG_DEBUG("Camera: pos(%.2f, %.2f, %.2f) front(%.2f, %.2f, %.2f)",
                            camPos.x, camPos.y, camPos.z,
                            camFront.x, camFront.y, camFront.z);
                        LOG_DEBUG("Camera: FOV=%.1f Near=%.2f Far=%.2f",
                            cam->GetFov(), cam->GetNearPlane(), cam->GetFarPlane());
                    }
                }
            }

            if (result != FrustumTestResult::FRUSTUM_OUT)
                return false;
        }
    }

    return true;
}
void Renderer::DrawScene()
{
    ComponentCamera* renderCamera = Application::GetInstance().camera->GetActiveCamera();
    ComponentCamera* cullingCamera = Application::GetInstance().camera->GetSceneCamera();

    if (cullingCamera == nullptr)
    {
        cullingCamera = renderCamera;
    }

    DrawScene(renderCamera, cullingCamera, true);
}

void Renderer::DrawScene(ComponentCamera* renderCamera, ComponentCamera* cullingCamera, bool drawEditorFeatures)
{
    ZoneScoped;
    GameObject* root = Application::GetInstance().scene->GetRoot();
    if (root == nullptr)
        return;

    if (!renderCamera)
        return;

    SelectionManager* selectionMgr = Application::GetInstance().selectionManager;
    const std::vector<GameObject*>& selectedObjects = selectionMgr->GetSelectedObjects();

    if (cullingCamera == nullptr)
    {
        cullingCamera = renderCamera;
    }

    // FIRST PASS: Render opaque objects and mark stencil for selected objects =====
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // Clear stencil buffer
    glClear(GL_STENCIL_BUFFER_BIT);

    // Draw all opaque objects, marking selected ones in stencil
    DrawGameObjectIterative(root, false, renderCamera, cullingCamera);

    // Only process selection highlighting if editor features are enabled
    if (drawEditorFeatures)
    {
        // Mark selected objects in stencil buffer
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);

        for (GameObject* selectedObj : selectedObjects)
        {
            if (!selectedObj->IsActive())
                continue;
            if (!IsGameObjectAndParentsActive(selectedObj))
                continue;

            Transform* transform = static_cast<Transform*>(selectedObj->GetComponent(ComponentType::TRANSFORM));
            if (transform == nullptr) continue;

            const std::vector<Component*>& meshComponents =
                selectedObj->GetComponentsOfType(ComponentType::MESH);

            for (Component* comp : meshComponents)
            {
                ComponentMesh* meshComp = static_cast<ComponentMesh*>(comp);
                if (meshComp->IsActive() && meshComp->HasMesh())
                {
                    const Mesh& mesh = meshComp->GetMesh();
                    glm::mat4 globalMatrix = transform->GetGlobalMatrix();

                    defaultShader->Use();
                    glUniformMatrix4fv(glGetUniformLocation(defaultShader->GetProgramID(), "model"),
                        1, GL_FALSE, glm::value_ptr(globalMatrix));
                    glUniformMatrix4fv(glGetUniformLocation(defaultShader->GetProgramID(), "view"),
                        1, GL_FALSE, glm::value_ptr(renderCamera->GetViewMatrix()));
                    glUniformMatrix4fv(glGetUniformLocation(defaultShader->GetProgramID(), "projection"),
                        1, GL_FALSE, glm::value_ptr(renderCamera->GetProjectionMatrix()));

                    DrawMesh(mesh);
                }
            }
        }

        // Restore color and depth writing
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glStencilMask(0x00);
    }

    //  SECOND PASS: Render transparent objects back-to-front =====
    std::vector<TransparentObject> transparentObjects;
    CollectTransparentObjects(root, transparentObjects);

    std::sort(transparentObjects.begin(), transparentObjects.end(),
        [](const TransparentObject& a, const TransparentObject& b) {
            return a.distanceToCamera > b.distanceToCamera;
        });

    for (const auto& transparentObj : transparentObjects)
    {
        DrawGameObjectIterative(transparentObj.gameObject, true, renderCamera, cullingCamera);
    }
    //  THIRD PASS: Draw outlines for selected objects =====
    if (drawEditorFeatures)
    {
        // Draw outline where stencil != 1 (around the selected object)
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);  
        glDepthMask(GL_FALSE);   

        outlineShader->Use();
        glUniformMatrix4fv(outlineUniforms.projection, 1, GL_FALSE,
            glm::value_ptr(renderCamera->GetProjectionMatrix()));
        glUniformMatrix4fv(outlineUniforms.view, 1, GL_FALSE,
            glm::value_ptr(renderCamera->GetViewMatrix()));
        outlineShader->SetVec3("outlineColor", glm::vec3(1.0f, 0.41f, 0.71f));

        float outlineThickness = 0.04f;
        outlineShader->SetFloat("outlineThickness", outlineThickness);

        for (GameObject* selectedObj : selectedObjects)
        {
            if (!selectedObj->IsActive())
                continue;
            if (!IsGameObjectAndParentsActive(selectedObj))
                continue;

            Transform* transform = static_cast<Transform*>(selectedObj->GetComponent(ComponentType::TRANSFORM));
            if (transform == nullptr) continue;

            const std::vector<Component*>& meshComponents =
                selectedObj->GetComponentsOfType(ComponentType::MESH);

            for (Component* comp : meshComponents)
            {
                ComponentMesh* meshComp = static_cast<ComponentMesh*>(comp);
                if (meshComp->IsActive() && meshComp->HasMesh())
                {
                    const Mesh& mesh = meshComp->GetMesh();
                    glm::mat4 globalMatrix = transform->GetGlobalMatrix();

                    glUniformMatrix4fv(outlineUniforms.model, 1, GL_FALSE,
                        glm::value_ptr(globalMatrix));

                    DrawMesh(mesh);
                }
            }
        }
    }

    // Restore render state
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glDepthMask(GL_TRUE);      
    glDepthFunc(GL_LESS);      
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    defaultShader->Use();
}

void Renderer::DrawAllAABBs(GameObject* gameObject)
{
    if (!gameObject || !gameObject->IsActive())
        return;

    ComponentMesh* meshComp = static_cast<ComponentMesh*>(
        gameObject->GetComponent(ComponentType::MESH));

    if (meshComp && meshComp->IsActive() && meshComp->HasMesh())
    {
        glm::vec3 worldMin, worldMax;
        meshComp->GetWorldAABB(worldMin, worldMax);
        DrawAABB(worldMin, worldMax, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    for (GameObject* child : gameObject->GetChildren())
    {
        DrawAllAABBs(child);
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


void Renderer::DrawGameObjectIterative(GameObject* gameObject,
    bool renderTransparentOnly,
    ComponentCamera* renderCamera,
    ComponentCamera* cullingCamera)
{
    std::stack<GameObject*> gameObjectStack;
    gameObjectStack.push(gameObject);

    while (!gameObjectStack.empty())
    {
        GameObject* currentObj = gameObjectStack.top();
        gameObjectStack.pop();

        if (!currentObj->IsActive())
            continue;

        Transform* transform = static_cast<Transform*>(
            currentObj->GetComponent(ComponentType::TRANSFORM));
        if (transform == nullptr) continue;

        if (cullingCamera &&
            cullingCamera->IsActive() &&
            cullingCamera->IsFrustumCullingEnabled())
        {
            if (ShouldCullGameObject(currentObj, cullingCamera->GetFrustum()))
            {
                continue;
            }
        }

        bool isTransparent = HasTransparency(currentObj);

        if (renderTransparentOnly != isTransparent)
        {
            // LIFO (Last In, First Out) to process children
            const auto& children = currentObj->GetChildren();
            for (auto it = children.rbegin(); it != children.rend(); ++it)
            {
                gameObjectStack.push(*it);
            }
            continue;
        }

        const glm::mat4& modelMatrix = transform->GetGlobalMatrix();

        Shader* currentShader = showZBuffer ? depthShader.get() : defaultShader.get();
        currentShader->Use();

        glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgramID(), "projection"),
            1, GL_FALSE, glm::value_ptr(renderCamera->GetProjectionMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgramID(), "view"),
            1, GL_FALSE, glm::value_ptr(renderCamera->GetViewMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgramID(), "model"),
            1, GL_FALSE, glm::value_ptr(modelMatrix));

        ComponentMaterial* material = static_cast<ComponentMaterial*>(
            currentObj->GetComponent(ComponentType::MATERIAL));

        bool materialBound = false;

        if (showZBuffer)
        {
            depthShader->SetFloat("nearPlane", renderCamera->GetNearPlane());
            depthShader->SetFloat("farPlane", renderCamera->GetFarPlane());
        }
        else
        {
            defaultShader->SetVec3("tintColor", glm::vec3(1.0f));

            bool hasTexture = (material && material->IsActive() && material->HasTexture());

            // Configure uniform hasTexture
            defaultShader->SetInt("hasTexture", hasTexture ? 1 : 0);

            // Configure light direction
            defaultShader->SetVec3("lightDir", glm::vec3(1.0f, -1.0f, -1.0f));

            if (hasTexture)
            {
                material->Use();  // Bind the material's texture
                materialBound = true;
                defaultShader->SetVec3("materialDiffuse", glm::vec3(1.0f));
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, 0); // No texture

                // Send diffuse colour of the material to the shader
                if (material && material->HasMaterialProperties())
                {
                    glm::vec4 diffuse = material->GetDiffuseColor();
                    defaultShader->SetVec3("materialDiffuse", glm::vec3(diffuse.r, diffuse.g, diffuse.b));
                }
                else
                {
                    // Default colour grey if no material is available
                    defaultShader->SetVec3("materialDiffuse", glm::vec3(0.6f, 0.6f, 0.6f));
                }
            }

            // Set the texture sampler uniform
            glUniform1i(defaultUniforms.texture1, 0);
        }

        const std::vector<Component*>& meshComponents =
            currentObj->GetComponentsOfType(ComponentType::MESH);

        ModuleEditor* editor = Application::GetInstance().editor.get();
        SelectionManager* selectionMgr = Application::GetInstance().selectionManager;

        bool shouldDrawNormals = false;
        if (editor && selectionMgr->IsSelected(currentObj))
        {
            shouldDrawNormals = true;
        }
        else if (editor)
        {
            GameObject* parent = currentObj->GetParent();
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

        bool showVertex = editor && editor->ShouldShowVertexNormals();
        bool showFace = editor && editor->ShouldShowFaceNormals();

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

        if (!renderTransparentOnly)
        {
            ComponentCamera* cam = static_cast<ComponentCamera*>(
                currentObj->GetComponent(ComponentType::CAMERA));

            if (cam && cam->IsActive() && cam->ShouldDrawFrustum())
            {
                glm::vec3 color = (cam == cullingCamera) ?
                    glm::vec3(0.0f, 1.0f, 0.0f) :
                    glm::vec3(1.0f, 1.0f, 0.0f);

                DrawCameraFrustum(cam, color);
            }
        }

        if (!showZBuffer)
        {
            
            ComponentCamera* editorCamera = Application::GetInstance().camera->GetActiveCamera();
            if (renderCamera == editorCamera)
            {
                Component* reverbCompBase = currentObj->GetComponent(ComponentType::REVERBZONE);
                if (reverbCompBase != nullptr)
                {
                    ReverbZone* zone = static_cast<ReverbZone*>(reverbCompBase);
                    if (zone->enabled)
                    {
                        
                        glm::vec3 worldCenter = glm::vec3(modelMatrix[3]);

                        if (zone->shape == ReverbZone::Shape::SPHERE)
                        {
                            DrawReverbSphere(worldCenter, zone->radius, glm::vec3(1.0f, 0.4f, 0.8f));
                        }
                        else 
                        {
                            
                            DrawReverbBox(modelMatrix, zone->extents, glm::vec3(1.0f, 0.4f, 0.8f));
                        }
                    }
                }
            }
        }

        if (materialBound)
            material->Unbind();
        else
            defaultTexture->Unbind();


		// LIFO((Last In, First Out) to process children
        const auto& children = currentObj->GetChildren();
        for (auto it = children.rbegin(); it != children.rend(); ++it)
        {
            gameObjectStack.push(*it);
        }
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
    ComponentCamera* camera = Application::GetInstance().camera->GetActiveCamera();
    if (!camera) return;

    lineShader->Use();
    glUniformMatrix4fv(glGetUniformLocation(lineShader->GetProgramID(), "projection"),
        1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(lineShader->GetProgramID(), "view"),
        1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(lineShader->GetProgramID(), "model"),
        1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    lineShader->SetVec3("tintColor", glm::vec3(0.0f, 0.5f, 1.0f));
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

    ComponentCamera* camera = Application::GetInstance().camera->GetActiveCamera();
    if (!camera) return;

    lineShader->Use();
    GLuint shaderProgram = lineShader->GetProgramID();

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    lineShader->SetVec3("tintColor", glm::vec3(0.0f, 1.0f, 0.5f));

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

ComponentCamera* Renderer::GetCamera() {
    return Application::GetInstance().camera->GetActiveCamera();
}

void Renderer::DrawCameraFrustum(ComponentCamera* camera, const glm::vec3& color)
{
    if (!camera || !camera->ShouldDrawFrustum())
        return;

    // Get frustum corners
    glm::vec3 corners[8];
    camera->GetFrustumCorners(corners);

    // Build line vertices for the frustum wireframe
    std::vector<float> lineVertices;
    lineVertices.reserve(24 * 3); // 12 edges * 2 points * 3 floats

    // Near plane (4 edges)
    for (int i = 0; i < 4; ++i)
    {
        int next = (i + 1) % 4;
        lineVertices.insert(lineVertices.end(), {
            corners[i].x, corners[i].y, corners[i].z,
            corners[next].x, corners[next].y, corners[next].z
            });
    }

    // Far plane (4 edges)
    for (int i = 4; i < 8; ++i)
    {
        int next = 4 + ((i - 4 + 1) % 4);
        lineVertices.insert(lineVertices.end(), {
            corners[i].x, corners[i].y, corners[i].z,
            corners[next].x, corners[next].y, corners[next].z
            });
    }

    // Connecting edges (4 edges from near to far)
    for (int i = 0; i < 4; ++i)
    {
        lineVertices.insert(lineVertices.end(), {
            corners[i].x, corners[i].y, corners[i].z,
            corners[i + 4].x, corners[i + 4].y, corners[i + 4].z
            });
    }

    // Create temporary VAO and VBO for frustum lines
    GLuint frustumVAO, frustumVBO;
    glGenVertexArrays(1, &frustumVAO);
    glGenBuffers(1, &frustumVBO);

    glBindVertexArray(frustumVAO);
    glBindBuffer(GL_ARRAY_BUFFER, frustumVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float),
        lineVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    // Get active camera for rendering
    ComponentCamera* activeCamera = GetCamera();
    if (!activeCamera) return;

    // Use line shader
    lineShader->Use();
    GLuint shaderProgram = lineShader->GetProgramID();

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(activeCamera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(activeCamera->GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    lineShader->SetVec3("color", color);

    // Draw the frustum
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);
    glLineWidth(1.0f);

    // Cleanup
    glBindVertexArray(0);
    glDeleteBuffers(1, &frustumVBO);
    glDeleteVertexArrays(1, &frustumVAO);

    defaultShader->Use();
}
bool Renderer::IsGameObjectAndParentsActive(GameObject* gameObject) const
{
    if (gameObject == nullptr)
        return false;

    if (!gameObject->IsActive())
        return false;

    GameObject* parent = gameObject->GetParent();
    while (parent != nullptr)
    {
        if (!parent->IsActive())
            return false;
        parent = parent->GetParent();
    }

    return true;
}

void Renderer::DrawRay(const glm::vec3& origin, const glm::vec3& direction,
    float length, const glm::vec3& color)
{
    glm::vec3 endPoint = origin + direction * length;

    std::vector<float> lineVertices = {
        origin.x, origin.y, origin.z,
        endPoint.x, endPoint.y, endPoint.z
    };

    GLuint rayVAO, rayVBO;
    glGenVertexArrays(1, &rayVAO);
    glGenBuffers(1, &rayVBO);

    glBindVertexArray(rayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float),
        lineVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    ComponentCamera* camera = GetCamera();
    if (!camera) return;

    lineShader->Use();
    GLuint shaderProgram = lineShader->GetProgramID();

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    if (colorLoc == -1)
        colorLoc = glGetUniformLocation(shaderProgram, "tintColor");

    if (colorLoc != -1)
        glUniform3fv(colorLoc, 1, glm::value_ptr(color));

    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, 2);
    glLineWidth(1.0f);

    glBindVertexArray(0);
    glDeleteBuffers(1, &rayVBO);
    glDeleteVertexArrays(1, &rayVAO);

    defaultShader->Use();
}

// Método para dibujar AABB de un GameObject
void Renderer::DrawAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color)
{
    glm::vec3 corners[8] = {
        glm::vec3(min.x, min.y, min.z), // 0
        glm::vec3(max.x, min.y, min.z), // 1
        glm::vec3(max.x, min.y, max.z), // 2
        glm::vec3(min.x, min.y, max.z), // 3
        glm::vec3(min.x, max.y, min.z), // 4
        glm::vec3(max.x, max.y, min.z), // 5
        glm::vec3(max.x, max.y, max.z), // 6
        glm::vec3(min.x, max.y, max.z)  // 7
    };

    std::vector<float> lineVertices;
    lineVertices.reserve(24 * 3);

    // Bottom face
    for (int i = 0; i < 4; ++i)
    {
        int next = (i + 1) % 4;
        lineVertices.insert(lineVertices.end(), {
            corners[i].x, corners[i].y, corners[i].z,
            corners[next].x, corners[next].y, corners[next].z
            });
    }

    // Top face
    for (int i = 4; i < 8; ++i)
    {
        int next = 4 + ((i - 4 + 1) % 4);
        lineVertices.insert(lineVertices.end(), {
            corners[i].x, corners[i].y, corners[i].z,
            corners[next].x, corners[next].y, corners[next].z
            });
    }

    // Vertical edges
    for (int i = 0; i < 4; ++i)
    {
        lineVertices.insert(lineVertices.end(), {
            corners[i].x, corners[i].y, corners[i].z,
            corners[i + 4].x, corners[i + 4].y, corners[i + 4].z
            });
    }

    GLuint aabbVAO, aabbVBO;
    glGenVertexArrays(1, &aabbVAO);
    glGenBuffers(1, &aabbVBO);

    glBindVertexArray(aabbVAO);
    glBindBuffer(GL_ARRAY_BUFFER, aabbVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float),
        lineVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    ComponentCamera* camera = GetCamera();
    if (!camera) return;

    lineShader->Use();
    GLuint shaderProgram = lineShader->GetProgramID();

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    if (colorLoc == -1) colorLoc = glGetUniformLocation(shaderProgram, "tintColor");
    if (colorLoc != -1) glUniform3fv(colorLoc, 1, glm::value_ptr(color));

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);
    glLineWidth(1.0f);

    glBindVertexArray(0);
    glDeleteBuffers(1, &aabbVBO);
    glDeleteVertexArrays(1, &aabbVAO);

    defaultShader->Use();
}

void Renderer::CreateGameFramebuffer(int width, int height)
{
    gameFramebufferWidth = width;
    gameFramebufferHeight = height;

    glGenFramebuffers(1, &gameFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gameFbo);

    glGenTextures(1, &gameTexture);
    glBindTexture(GL_TEXTURE_2D, gameTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gameTexture, 0);

    glGenRenderbuffers(1, &gameRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, gameRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gameRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_DEBUG("ERROR::FRAMEBUFFER:: Game framebuffer is not complete!");
        LOG_CONSOLE("ERROR: Failed to create game framebuffer");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ResizeGameFramebuffer(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    if (gameFramebufferWidth == width && gameFramebufferHeight == height)
        return;

    gameFramebufferWidth = width;
    gameFramebufferHeight = height;

    // Delete old framebuffer resources
    if (gameTexture != 0)
        glDeleteTextures(1, &gameTexture);
    if (gameRbo != 0)
        glDeleteRenderbuffers(1, &gameRbo);
    if (gameFbo != 0)
        glDeleteFramebuffers(1, &gameFbo);

    CreateGameFramebuffer(width, height);
}

void Renderer::BindGameFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, gameFbo);
    glViewport(0, 0, gameFramebufferWidth, gameFramebufferHeight);
}

void Renderer::DrawReverbSphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments)
{
    if (segments < 4) segments = 4;

    std::vector<float> verts;
    verts.reserve(segments * 6 * 3); // three rings, segments lines (two points each), 3 floats

    auto addCircle = [&](const glm::vec3& up, const glm::vec3& right, float r) {
        for (int i = 0; i < segments; ++i)
        {
            float a0 = (float)i / segments * glm::two_pi<float>();
            float a1 = (float)(i + 1) / segments * glm::two_pi<float>();

            glm::vec3 p0 = center + (right * cosf(a0) + up * sinf(a0)) * r;
            glm::vec3 p1 = center + (right * cosf(a1) + up * sinf(a1)) * r;

            verts.insert(verts.end(), { p0.x, p0.y, p0.z, p1.x, p1.y, p1.z });
        }
    };

    // Choose orthonormal axes for rings
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 forward(0.0f, 0.0f, 1.0f);

    // horizontal ring (XZ)
    addCircle(right, forward, radius);
    // vertical ring (XY)
    addCircle(right, up, radius);
    // vertical ring (YZ)
    addCircle(forward, up, radius);

    if (verts.empty()) return;

    // Create temporary VAO/VBO
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    ComponentCamera* camera = GetCamera();
    if (!camera)
    {
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return;
    }

    lineShader->Use();
    GLuint program = lineShader->GetProgramID();
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    // set color uniform (some shaders use 'color' or 'tintColor')
    GLint colorLoc = glGetUniformLocation(program, "color");
    if (colorLoc == -1) colorLoc = glGetUniformLocation(program, "tintColor");
    if (colorLoc != -1) glUniform3fv(colorLoc, 1, glm::value_ptr(color));

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, (GLsizei)(verts.size() / 3));
    glLineWidth(1.0f);

    // cleanup
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    defaultShader->Use();
}

void Renderer::DrawReverbBox(const glm::mat4& modelMatrix, const glm::vec3& extents, const glm::vec3& color)
{
    // Build 8 local corners
    glm::vec3 localCorners[8] = {
        { -extents.x, -extents.y, -extents.z },
        {  extents.x, -extents.y, -extents.z },
        {  extents.x, -extents.y,  extents.z },
        { -extents.x, -extents.y,  extents.z },
        { -extents.x,  extents.y, -extents.z },
        {  extents.x,  extents.y, -extents.z },
        {  extents.x,  extents.y,  extents.z },
        { -extents.x,  extents.y,  extents.z }
    };

    // Transform to world space using modelMatrix
    glm::vec3 worldCorners[8];
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 wc = modelMatrix * glm::vec4(localCorners[i], 1.0f);
        worldCorners[i] = glm::vec3(wc);
    }

    // Build line list for edges (12 edges -> 24 points)
    std::vector<float> verts;
    auto pushEdge = [&](int a, int b) {
        verts.insert(verts.end(), {
            worldCorners[a].x, worldCorners[a].y, worldCorners[a].z,
            worldCorners[b].x, worldCorners[b].y, worldCorners[b].z
        });
    };

    // bottom
    pushEdge(0,1); pushEdge(1,2); pushEdge(2,3); pushEdge(3,0);
    // top
    pushEdge(4,5); pushEdge(5,6); pushEdge(6,7); pushEdge(7,4);
    // verticals
    pushEdge(0,4); pushEdge(1,5); pushEdge(2,6); pushEdge(3,7);

    if (verts.empty()) return;

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    ComponentCamera* camera = GetCamera();
    if (!camera)
    {
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return;
    }

    lineShader->Use();
    GLuint program = lineShader->GetProgramID();
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    GLint colorLoc = glGetUniformLocation(program, "color");
    if (colorLoc == -1) colorLoc = glGetUniformLocation(program, "tintColor");
    if (colorLoc != -1) glUniform3fv(colorLoc, 1, glm::value_ptr(color));

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, (GLsizei)(verts.size() / 3));
    glLineWidth(1.0f);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    defaultShader->Use();
}