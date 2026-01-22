#include "ComponentCollider.h"

ComponentCollider::ComponentCollider(GameObject* owner, ComponentType type) 
    : Component(owner, type) 
{
    // Inicialización de la base
}

ComponentCollider::~ComponentCollider() 
{
    // Muy importante: limpiar la forma de Bullet para evitar fugas de memoria
    if (shape) 
    {
        delete shape;
        shape = nullptr;
    }
}