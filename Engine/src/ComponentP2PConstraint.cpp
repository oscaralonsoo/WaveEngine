#include "ComponentP2PConstraint.h"
#include "GameObject.h"
#include "ComponentRigidBody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include <imgui.h>
#include "Transform.h"

ComponentP2PConstraint::ComponentP2PConstraint(GameObject* owner) 
    : Component(owner, ComponentType::CONSTRAINT_P2P) {}

ComponentP2PConstraint::~ComponentP2PConstraint() {
    RemoveConstraint();
}

void ComponentP2PConstraint::Start() {
    CreateConstraint();
}

void ComponentP2PConstraint::CreateConstraint() {
    RemoveConstraint();

    ComponentRigidBody* rbA = (ComponentRigidBody*)owner->GetComponent(ComponentType::RIGIDBODY);
    if (!rbA || !targetObject) return;

    ComponentRigidBody* rbB = (ComponentRigidBody*)targetObject->GetComponent(ComponentType::RIGIDBODY);
    if (!rbB) return;

    btRigidBody* bodyA = rbA->GetRigidBody();
    btRigidBody* bodyB = rbB->GetRigidBody();

    if (bodyA && bodyB) {
        btVector3 btPivotA(pivotA.x, pivotA.y, pivotA.z);
        btVector3 btPivotB(pivotB.x, pivotB.y, pivotB.z);

        constraint = new btPoint2PointConstraint(*bodyA, *bodyB, btPivotA, btPivotB);
        
        // Configuración opcional
        constraint->m_setting.m_tau = tau;
        constraint->m_setting.m_damping = damping;
        constraint->m_setting.m_impulseClamp = impulseClamp;

        Application::GetInstance().physics->GetWorld()->addConstraint(constraint, true);
    }
}

void ComponentP2PConstraint::RemoveConstraint() {
    if (constraint) {
        if (Application::GetInstance().physics->GetWorld()) {
            Application::GetInstance().physics->GetWorld()->removeConstraint(constraint);
        }
        delete constraint;
        constraint = nullptr;
    }
}

void ComponentP2PConstraint::OnEditor() {
    if (ImGui::CollapsingHeader("Point-to-Point Constraint", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        // 1. Selector de Target (Cuerpo B)
        std::string targetName = targetObject ? targetObject->GetName() : "None (Select below)";
        ImGui::Text("Connected To: %s", targetName.c_str());

        if (ImGui::BeginCombo("Select Target", targetName.c_str())) {
            
            // Función lambda recursiva para recorrer el árbol y mostrar opciones
            std::function<void(GameObject*)> ShowObjectSelector = [&](GameObject* obj) {
                if (!obj || obj->GetName() == "Root") { // Saltamos el root si quieres
                    for (auto child : obj->GetChildren()) ShowObjectSelector(child);
                    return;
                }

                if (obj != owner && obj->GetComponent(ComponentType::RIGIDBODY)) {
                    bool isSelected = (targetObject == obj);
                    if (ImGui::Selectable(obj->GetName().c_str(), isSelected)) {
                        targetObject = obj;
                        CreateConstraint();
                    }
                }

                for (auto child : obj->GetChildren()) {
                    ShowObjectSelector(child);
                }
            };

            ShowObjectSelector(Application::GetInstance().scene->GetRoot());
            
            ImGui::EndCombo();
        }

        ImGui::Separator();

        // 2. Edición de Pivots
        ImGui::Text("Pivots (Local Space)");
        if (ImGui::DragFloat3("Pivot in A (This)", &pivotA[0], 0.05f)) {
            CreateConstraint();
        }
        if (ImGui::DragFloat3("Pivot in B (Target)", &pivotB[0], 0.05f)) {
            CreateConstraint();
        }

        // 3. Botón de ayuda para auto-posicionar
        if (ImGui::Button("Auto-Align Pivots")) {
            // Lógica simple: poner los pivots donde se tocan visualmente en el momento actual
            // Esto requiere cálculos de transformación global a local
            CalculateAutoPivots(); 
            CreateConstraint();
        }
    }
}

void ComponentP2PConstraint::CalculateAutoPivots() {
    if (!targetObject) return;

    // Obtenemos los Transforms mediante GetComponent
    Transform* transA = (Transform*)owner->GetComponent(ComponentType::TRANSFORM);
    Transform* transB = (Transform*)targetObject->GetComponent(ComponentType::TRANSFORM);

    if (!transA || !transB) return;

    // 1. Calcular punto medio en el mundo
    glm::vec3 posA = transA->GetGlobalPosition();
    glm::vec3 posB = transB->GetGlobalPosition();
    glm::vec4 midPointWorld = glm::vec4((posA + posB) * 0.5f, 1.0f);

    // 2. Convertir de World Space a Local Space usando la matriz inversa
    // Pivot A
    glm::mat4 invA = glm::inverse(transA->GetGlobalMatrix());
    pivotA = glm::vec3(invA * midPointWorld);

    // Pivot B
    glm::mat4 invB = glm::inverse(transB->GetGlobalMatrix());
    pivotB = glm::vec3(invB * midPointWorld);
}