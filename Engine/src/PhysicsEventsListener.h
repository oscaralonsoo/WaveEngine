#pragma once
#include "Component.h"
#include <PxPhysicsAPI.h>

class Rigidbody;
class Collider;

class PhysicsEventsListener
{
public:

    virtual ~PhysicsEventsListener() {}

    virtual void OnCollisionEnter(Rigidbody* other) {}
    virtual void OnCollisionStay(Rigidbody* other) {}
    virtual void OnCollisionExit(Rigidbody* other) {}

    virtual void OnTriggerEnter(Rigidbody* other) {}
    virtual void OnTriggerStay(Rigidbody* other) {}
    virtual void OnTriggerExit(Rigidbody* other) {}
};