#include "ComponentParticleSystem.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleCamera.h"
#include "ComponentCamera.h"
#include "Transform.h"
#include "ModuleResources.h" 
#include "ResourceTexture.h"
#include "LibraryManager.h"
#include "Time.h" 
#include <fstream>
#include <iomanip> 
#include <imgui.h>
#include <glad/glad.h> 
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

ComponentParticleSystem::ComponentParticleSystem(GameObject* owner)
    : Component(owner, ComponentType::PARTICLE)
{
    name = "Particle System";
    emitter = new EmitterInstance();
    emitter->Init(); // Initialize default modules
}

ComponentParticleSystem::~ComponentParticleSystem() {
    // Release texture resource reference
    if (textureResourceUID != 0) {
        Application::GetInstance().resources->ReleaseResource(textureResourceUID);
    }
    // CleanUp memory
    if (emitter) {
        delete emitter;
        emitter = nullptr;
    }
}

void ComponentParticleSystem::Update() {
    // Update feedback message timer
    if (feedbackTimer > 0.0f) {
        feedbackTimer -= Application::GetInstance().time->GetRealDeltaTime();
    }

    // Connection with transform and prewarm
    Transform* trans = dynamic_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (trans) {
        const glm::mat4& globalMatrix = trans->GetGlobalMatrix();
        glm::vec3 globalPos = glm::vec3(globalMatrix[3]); // Extract translation

        // Pass global position to the emitter, used for RateOverDistance and World Space
        emitter->ownerPosition = globalPos;

        // Update Spawner module position (Legacy sync)
        for (auto mod : emitter->modules) {
            if (mod->type == ParticleModuleType::SPAWNER) {
                static_cast<ModuleEmitterSpawn*>(mod)->spawnPosition = globalPos;
            }
        }
    }

    // Only update simulation on PLAY mode
    if (Application::GetInstance().GetPlayState() != Application::PlayState::PLAYING) return;

    // Prewarm logic, instant simulation at start
    if (emitter->prewarm && emitter->systemTime == 0.0f) {
        float simStep = 0.1f;
        float simTime = 2.0f; // Simulate 2 seconds instantly
        for (float t = 0; t < simTime; t += simStep) {
            emitter->Update(simStep);
        }
    }

    float dt = Application::GetInstance().time->GetDeltaTime();
    emitter->Update(dt);
}

void ComponentParticleSystem::Draw(ComponentCamera* camera) {
    if (!active || !emitter) return;

    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Setup Legacy Matrices
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(camera->GetProjectionMatrix()));

    glMatrixMode(GL_MODELVIEW);
    glm::mat4 viewMatrix = camera->GetViewMatrix();
    glLoadMatrixf(glm::value_ptr(viewMatrix));

    // Simulation space logic in render
    if (emitter->simulationSpace == SimulationSpace::LOCAL) {
        // In LOCAL, we apply the object's matrix. Particles move with it
        Transform* trans = dynamic_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
        if (trans) {
            glMultMatrixf(glm::value_ptr(trans->GetGlobalMatrix()));
        }
    }
    // In WORLD, we don't apply the object's matrix Identity, because particles already have absolute world positions calculated in Update()
    // Draw particles
    emitter->Draw(camera->GetPosition());
}

void ComponentParticleSystem::OnEditor() {
    if (ImGui::CollapsingHeader("Particle System", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Control buttons
        ImGui::Checkbox("Active", &active);
        ImGui::SameLine();
        if (ImGui::Button("Reset Simulation")) emitter->Reset(); // Clears particles in the scene
        ImGui::SameLine();
        if (ImGui::Button("Reset Values")) emitter->ResetValues(); // Resets to default

        // Selector Texture
        ImGui::Separator();
        ImGui::Text("Texture & Animation");
        std::string currentTexName = emitter->texturePath.empty() ? "None" : emitter->texturePath;
        // Strip path to show only filename
        size_t lastSlash = currentTexName.find_last_of("/\\");
        if (lastSlash != std::string::npos) currentTexName = currentTexName.substr(lastSlash + 1);

        if (ImGui::BeginCombo("Texture", currentTexName.c_str())) {
            const auto& allResources = Application::GetInstance().resources->GetAllResources();
            if (ImGui::Selectable("None", emitter->texturePath.empty())) SetTexture("");
            for (const auto& pair : allResources) {
                Resource* res = pair.second;
                if (res->GetType() == Resource::TEXTURE) {
                    std::string path = res->GetAssetFile();
                    std::string name = path;
                    size_t slash = name.find_last_of("/\\");
                    if (slash != std::string::npos) name = name.substr(slash + 1);
                    bool isSelected = (emitter->texturePath == path);
                    if (ImGui::Selectable(name.c_str(), isSelected)) SetTexture(path);
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // Animation Settings
        ImGui::DragInt("Rows", &emitter->textureRows, 0.1f, 1, 16);
        ImGui::DragInt("Columns", &emitter->textureCols, 0.1f, 1, 16);
        ImGui::DragFloat("Animation Speed", &emitter->animationSpeed, 0.05f, 0.0f, 10.0f, "%.2fx");
        ImGui::Checkbox("Loop Animation", &emitter->animLoop);

        // Global Settings
        ImGui::Separator();
        ImGui::Text("System Settings");

        const char* spaceItems[] = { "Local", "World" };
        int currentSpace = (int)emitter->simulationSpace;
        if (ImGui::Combo("Simulation Space", &currentSpace, spaceItems, IM_ARRAYSIZE(spaceItems))) {
            emitter->simulationSpace = (SimulationSpace)currentSpace;
        }

        ImGui::Checkbox("Prewarm", &emitter->prewarm);
        ImGui::DragInt("Max Particles", &emitter->maxParticles, 1, 0, 10000);
        ImGui::DragFloat("Emission Rate (Time)", &emitter->emissionRate, 0.1f, 0.0f, 1000.0f);
        ImGui::DragFloat("Emission Rate (Dist)", &emitter->emissionRateDistance, 0.1f, 0.0f, 100.0f);

        // Bursts UI
        if (ImGui::TreeNode("Bursts Configuration")) {
            if (ImGui::Button("+ Add Burst")) {
                emitter->bursts.push_back(Burst());
            }

            for (int i = 0; i < emitter->bursts.size(); ++i) {
                ImGui::PushID(i);
                ImGui::Text("Burst %d", i);
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    emitter->bursts.erase(emitter->bursts.begin() + i);
                    ImGui::PopID();
                    continue;
                }

                ImGui::DragFloat("Time", &emitter->bursts[i].time, 0.1f, 0.0f, 10.0f);
                ImGui::DragInt("Count", &emitter->bursts[i].count, 1, 1, 1000);
                ImGui::DragInt("Cycles", &emitter->bursts[i].cycles, 1, 0, 100);
                ImGui::DragFloat("Interval", &emitter->bursts[i].repeatInterval, 0.1f, 0.0f, 10.0f);
                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        ImGui::Checkbox("Additive Blending (Glow)", &emitter->additiveBlending);

        // Module settings
        ModuleEmitterSpawn* spawner = nullptr;
        ModuleEmitterNoise* noise = nullptr;

        // Find modules
        for (auto m : emitter->modules) {
            if (m->type == ParticleModuleType::SPAWNER) spawner = (ModuleEmitterSpawn*)m;
            if (m->type == ParticleModuleType::NOISE) noise = (ModuleEmitterNoise*)m;
        }

        // Spawner UI
        if (spawner && ImGui::CollapsingHeader("Spawner Module", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* shapeItems[] = { "Box", "Sphere", "Cone", "Circle" };
            int currentShape = (int)spawner->shape;
            if (ImGui::Combo("Emitter Shape", &currentShape, shapeItems, IM_ARRAYSIZE(shapeItems))) {
                spawner->shape = (EmitterShape)currentShape;
            }

            // Dynamic parameters based on shape
            if (spawner->shape == EmitterShape::BOX) {
                ImGui::DragFloat3("Box Area", &spawner->emissionArea.x, 0.1f, 0.0f, 10.0f);
            }
            else if (spawner->shape == EmitterShape::SPHERE) {
                ImGui::DragFloat("Radius", &spawner->emissionRadius, 0.1f, 0.0f, 10.0f);
                ImGui::Checkbox("Emit From Shell", &spawner->emitFromShell);
            }
            else if (spawner->shape == EmitterShape::CONE) {
                ImGui::DragFloat("Base Radius", &spawner->coneRadius, 0.1f);
                ImGui::SliderFloat("Cone Angle", &spawner->coneAngle, 0.0f, 90.0f);
                ImGui::Checkbox("Emit From Shell", &spawner->emitFromShell);
            }
            else if (spawner->shape == EmitterShape::CIRCLE) {
                ImGui::DragFloat("Circle Radius", &spawner->circleRadius, 0.1f);
                ImGui::Checkbox("Emit From Edge", &spawner->emitFromShell);
            }

            ImGui::Separator();
            ImGui::Text("Lifecycle & Physics");
            ImGui::DragFloat2("Lifetime (Min/Max)", &spawner->lifetimeMin, 0.1f, 0.1f, 10.0f);
            ImGui::DragFloat2("Speed (Min/Max)", &spawner->speedMin, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("Size Start", &spawner->sizeStart, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat("Size End", &spawner->sizeEnd, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat2("Spin Speed (Min/Max)", &spawner->rotationSpeedMin, 1.0f, -360.0f, 360.0f);

            // UI for color gradients
            ImGui::Separator();
            ImGui::Text("Color over Lifetime");

            // Simple Mode
            ImGui::ColorEdit4("Start Color", &spawner->colorStart.r);
            ImGui::ColorEdit4("End Color", &spawner->colorEnd.r);

            // Advanced mode, button to add keys
            if (ImGui::TreeNode("Advanced Gradient Editor")) {
                if (ImGui::Button("Add Color Key")) {
                    ColorKey key;
                    key.time = 0.5f;
                    key.color = glm::vec4(1.0f);
                    spawner->colorGradient.push_back(key);

                    std::sort(spawner->colorGradient.begin(), spawner->colorGradient.end(),
                        [](const ColorKey& a, const ColorKey& b) { return a.time < b.time; });
                }

                bool gradientChanged = false;

                for (int i = 0; i < spawner->colorGradient.size(); i++) {
                    ImGui::PushID(i + 100);

                    if (ImGui::SliderFloat("Time", &spawner->colorGradient[i].time, 0.0f, 1.0f)) {
                        gradientChanged = true;
                    }
                    ImGui::ColorEdit4("Color", &spawner->colorGradient[i].color.r);

                    if (ImGui::Button("Remove")) {
                        spawner->colorGradient.erase(spawner->colorGradient.begin() + i);
                        ImGui::PopID();
                        continue;
                    }
                    ImGui::PopID();
                }

                if (gradientChanged) {
                    std::sort(spawner->colorGradient.begin(), spawner->colorGradient.end(),
                        [](const ColorKey& a, const ColorKey& b) { return a.time < b.time; });
                }
                ImGui::TreePop();
            }
        }

        // Noise module UI
        if (noise && ImGui::CollapsingHeader("Noise Module")) {
            ImGui::Checkbox("Enable Noise Turbulence", &noise->active);
            if (noise->active) {
                ImGui::DragFloat("Noise Strength", &noise->strength, 0.1f, 0.0f, 50.0f);
                ImGui::DragFloat("Noise Frequency", &noise->frequency, 0.01f, 0.0f, 10.0f);
            }
        }

        // Movement module UI
        if (ImGui::CollapsingHeader("Movement Module")) {
            ModuleEmitterMovement* mov = nullptr;
            for (auto m : emitter->modules) if (m->type == ParticleModuleType::MOVEMENT) mov = (ModuleEmitterMovement*)m;
            if (mov) ImGui::DragFloat3("Gravity Vector", &mov->gravity.x, 0.1f);
        }

        // Presets
        ImGui::Separator();
        ImGui::Text("Presets (.particle)");
        static char buf[64] = "new_effect";
        ImGui::InputText("Filename", buf, 64);

        // Path using LibraryManager
        std::string assetsRoot = LibraryManager::GetAssetsRoot();
        if (assetsRoot.back() != '/' && assetsRoot.back() != '\\') assetsRoot += "/";
        std::string particleFolder = assetsRoot + "Particles/";

        if (ImGui::Button("Save Preset")) {
            std::string path = particleFolder + std::string(buf) + ".particle";
            SaveParticleFile(path);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Preset")) {
            std::string path = particleFolder + std::string(buf) + ".particle";
            LoadParticleFile(path);
        }

        // Feedback
        if (feedbackTimer > 0.0f) {
            ImGui::Separator();
            ImVec4 color = feedbackIsError ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
            ImGui::TextColored(color, "%s", feedbackMessage.c_str());
        }
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Particles Alive: %d", (int)emitter->particles.size());
    }
}

void ComponentParticleSystem::SetTexture(const std::string& path) {
    if (!emitter) return;

    // Release previous resource
    if (textureResourceUID != 0) {
        Application::GetInstance().resources->ReleaseResource(textureResourceUID);
        textureResourceUID = 0;
        emitter->textureID = 0;
    }
    emitter->texturePath = path;
    if (path.empty()) return;

    // Search and Request new resource
    const auto& allResources = Application::GetInstance().resources->GetAllResources();
    for (const auto& pair : allResources) {
        Resource* res = pair.second;
        if (res->GetType() == Resource::TEXTURE && res->GetAssetFile() == path) {
            // Loaded form VRAM
            Resource* loadedRes = Application::GetInstance().resources->RequestResource(res->GetUID());
            if (loadedRes) {
                const ResourceTexture* texRes = static_cast<const ResourceTexture*>(loadedRes);
                emitter->textureID = texRes->GetGPU_ID();
                textureResourceUID = res->GetUID();
            }
            return;
        }
    }
}

// Json
void ComponentParticleSystem::Serialize(nlohmann::json& componentObj) const {
    componentObj["type"] = static_cast<int>(ComponentType::PARTICLE);
    componentObj["active"] = active;
    componentObj["maxParticles"] = emitter->maxParticles;
    componentObj["emissionRate"] = emitter->emissionRate;
    componentObj["emissionRateDist"] = emitter->emissionRateDistance;
    componentObj["simulationSpace"] = (int)emitter->simulationSpace;
    componentObj["prewarm"] = emitter->prewarm;
    componentObj["texturePath"] = emitter->texturePath;

    componentObj["additive"] = emitter->additiveBlending;
    componentObj["textureRows"] = emitter->textureRows;
    componentObj["textureCols"] = emitter->textureCols;
    componentObj["animSpeed"] = emitter->animationSpeed;
    componentObj["animLoop"] = emitter->animLoop;

    // Serialize Bursts
    nlohmann::json burstsJson = nlohmann::json::array();
    for (const auto& b : emitter->bursts) {
        burstsJson.push_back({
            {"time", b.time}, {"count", b.count}, {"cycles", b.cycles}, {"interval", b.repeatInterval}
            });
    }
    componentObj["bursts"] = burstsJson;

    for (auto mod : emitter->modules) {
        if (mod->type == ParticleModuleType::SPAWNER) {
            ModuleEmitterSpawn* s = static_cast<ModuleEmitterSpawn*>(mod);
            componentObj["shape"] = static_cast<int>(s->shape);
            componentObj["emissionArea"] = { s->emissionArea.x, s->emissionArea.y, s->emissionArea.z };
            componentObj["emissionRadius"] = s->emissionRadius;
            componentObj["coneRadius"] = s->coneRadius;
            componentObj["coneAngle"] = s->coneAngle;
            componentObj["circleRadius"] = s->circleRadius;
            componentObj["emitFromShell"] = s->emitFromShell;

            componentObj["colorStart"] = { s->colorStart.r, s->colorStart.g, s->colorStart.b, s->colorStart.a };
            componentObj["colorEnd"] = { s->colorEnd.r, s->colorEnd.g, s->colorEnd.b, s->colorEnd.a };

            // Serialize Gradient
            nlohmann::json gradJson = nlohmann::json::array();
            for (const auto& k : s->colorGradient) {
                gradJson.push_back({
                    {"time", k.time},
                    {"color", {k.color.r, k.color.g, k.color.b, k.color.a}}
                    });
            }
            componentObj["colorGradient"] = gradJson;

            componentObj["sizeStart"] = s->sizeStart;
            componentObj["sizeEnd"] = s->sizeEnd;
            componentObj["rotSpeedMin"] = s->rotationSpeedMin;
            componentObj["rotSpeedMax"] = s->rotationSpeedMax;
            componentObj["lifetimeMin"] = s->lifetimeMin;
            componentObj["lifetimeMax"] = s->lifetimeMax;
            componentObj["speedMin"] = s->speedMin;
            componentObj["speedMax"] = s->speedMax;
        }
        else if (mod->type == ParticleModuleType::MOVEMENT) {
            ModuleEmitterMovement* m = static_cast<ModuleEmitterMovement*>(mod);
            componentObj["gravity"] = { m->gravity.x, m->gravity.y, m->gravity.z };
        }
        else if (mod->type == ParticleModuleType::NOISE) {
            ModuleEmitterNoise* n = static_cast<ModuleEmitterNoise*>(mod);
            componentObj["noiseActive"] = n->active;
            componentObj["noiseStrength"] = n->strength;
            componentObj["noiseFreq"] = n->frequency;
        }
    }
}

void ComponentParticleSystem::Deserialize(const nlohmann::json& componentObj) {
    if (componentObj.contains("active")) active = componentObj["active"];
    if (componentObj.contains("maxParticles")) emitter->maxParticles = componentObj["maxParticles"];
    if (componentObj.contains("emissionRate")) emitter->emissionRate = componentObj["emissionRate"];
    if (componentObj.contains("emissionRateDist")) emitter->emissionRateDistance = componentObj["emissionRateDist"];
    if (componentObj.contains("simulationSpace")) emitter->simulationSpace = (SimulationSpace)componentObj["simulationSpace"];
    if (componentObj.contains("prewarm")) emitter->prewarm = componentObj["prewarm"];

    if (componentObj.contains("texturePath")) SetTexture(componentObj["texturePath"]);

    if (componentObj.contains("additive")) emitter->additiveBlending = componentObj["additive"];
    if (componentObj.contains("textureRows")) emitter->textureRows = componentObj["textureRows"];
    if (componentObj.contains("textureCols")) emitter->textureCols = componentObj["textureCols"];
    if (componentObj.contains("animSpeed")) emitter->animationSpeed = componentObj["animSpeed"];
    if (componentObj.contains("animLoop")) emitter->animLoop = componentObj["animLoop"];

    // Deserialize Bursts
    if (componentObj.contains("bursts")) {
        emitter->bursts.clear();
        for (const auto& b : componentObj["bursts"]) {
            Burst burst;
            burst.time = b["time"]; burst.count = b["count"];
            burst.cycles = b["cycles"]; burst.repeatInterval = b["interval"];
            emitter->bursts.push_back(burst);
        }
    }

    for (auto mod : emitter->modules) {
        if (mod->type == ParticleModuleType::SPAWNER) {
            ModuleEmitterSpawn* s = static_cast<ModuleEmitterSpawn*>(mod);
            if (componentObj.contains("shape")) s->shape = static_cast<EmitterShape>(componentObj["shape"]);
            if (componentObj.contains("emissionRadius")) s->emissionRadius = componentObj["emissionRadius"];
            if (componentObj.contains("coneRadius")) s->coneRadius = componentObj["coneRadius"];
            if (componentObj.contains("coneAngle")) s->coneAngle = componentObj["coneAngle"];
            if (componentObj.contains("circleRadius")) s->circleRadius = componentObj["circleRadius"];
            if (componentObj.contains("emitFromShell")) s->emitFromShell = componentObj["emitFromShell"];

            if (componentObj.contains("colorStart")) { auto c = componentObj["colorStart"]; s->colorStart = glm::vec4(c[0], c[1], c[2], c[3]); }
            if (componentObj.contains("colorEnd")) { auto c = componentObj["colorEnd"]; s->colorEnd = glm::vec4(c[0], c[1], c[2], c[3]); }

            // Gradient
            if (componentObj.contains("colorGradient")) {
                s->colorGradient.clear();
                for (const auto& k : componentObj["colorGradient"]) {
                    ColorKey key;
                    key.time = k["time"];
                    auto c = k["color"];
                    key.color = glm::vec4(c[0], c[1], c[2], c[3]);
                    s->colorGradient.push_back(key);
                }
            }

            if (componentObj.contains("sizeStart")) s->sizeStart = componentObj["sizeStart"];
            if (componentObj.contains("sizeEnd")) s->sizeEnd = componentObj["sizeEnd"];
            if (componentObj.contains("rotSpeedMin")) s->rotationSpeedMin = componentObj["rotSpeedMin"];
            if (componentObj.contains("rotSpeedMax")) s->rotationSpeedMax = componentObj["rotSpeedMax"];

            if (componentObj.contains("lifetimeMin")) s->lifetimeMin = componentObj["lifetimeMin"];
            if (componentObj.contains("lifetimeMax")) s->lifetimeMax = componentObj["lifetimeMax"];
            if (componentObj.contains("speedMin")) s->speedMin = componentObj["speedMin"];
            if (componentObj.contains("speedMax")) s->speedMax = componentObj["speedMax"];
            if (componentObj.contains("emissionArea")) { auto a = componentObj["emissionArea"]; s->emissionArea = glm::vec3(a[0], a[1], a[2]); }
        }
        else if (mod->type == ParticleModuleType::MOVEMENT) {
            ModuleEmitterMovement* m = static_cast<ModuleEmitterMovement*>(mod);
            if (componentObj.contains("gravity")) { auto g = componentObj["gravity"]; m->gravity = glm::vec3(g[0], g[1], g[2]); }
        }
        else if (mod->type == ParticleModuleType::NOISE) {
            ModuleEmitterNoise* n = static_cast<ModuleEmitterNoise*>(mod);
            if (componentObj.contains("noiseActive")) n->active = componentObj["noiseActive"];
            if (componentObj.contains("noiseStrength")) n->strength = componentObj["noiseStrength"];
            if (componentObj.contains("noiseFreq")) n->frequency = componentObj["noiseFreq"];
        }
    }
}

void ComponentParticleSystem::SaveParticleFile(const std::string& path) {
    std::filesystem::path filepath(path);
    std::filesystem::path dir = filepath.parent_path();
    // Check directory exists
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    nlohmann::json jsonFile;
    Serialize(jsonFile);
    std::ofstream file(path);
    if (file.is_open()) {
        file << std::setw(4) << jsonFile << std::endl;
        file.close();
        feedbackMessage = "Success: Saved " + filepath.filename().string();
        feedbackIsError = false;
        feedbackTimer = 3.0f;
        LOG_CONSOLE("Saved particle preset to %s", path.c_str());
    }
    else {
        feedbackMessage = "Error: Could not save file!";
        feedbackIsError = true;
        feedbackTimer = 3.0f;
        LOG_CONSOLE("ERROR: Could not save to %s", path.c_str());
    }
}

void ComponentParticleSystem::LoadParticleFile(const std::string& path) {
    std::ifstream file(path);
    if (file.is_open()) {
        nlohmann::json jsonFile;
        file >> jsonFile;
        Deserialize(jsonFile);
        file.close();
        std::filesystem::path filepath(path);
        feedbackMessage = "Success: Loaded " + filepath.filename().string();
        feedbackIsError = false;
        feedbackTimer = 3.0f;
        LOG_CONSOLE("Loaded particle preset from %s", path.c_str());
    }
    else {
        feedbackMessage = "Error: File not found!";
        feedbackIsError = true;
        feedbackTimer = 3.0f;
        LOG_CONSOLE("ERROR: Could not load %s", path.c_str());
    }
}