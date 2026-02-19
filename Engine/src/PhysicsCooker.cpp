#include "PhysicsCooker.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "cooking/PxCooking.h"

physx::PxConvexMesh* PhysicsCooker::CookConvex(const float* vertexBuffer, uint32_t vertexCount, uint32_t stride)
{
    auto* physics = Application::GetInstance().physics.get()->GetPhysics();

    physx::PxConvexMeshDesc convexDesc;
    convexDesc.setToDefault();
    convexDesc.points.count = vertexCount;
    convexDesc.points.stride = stride;
    convexDesc.points.data = vertexBuffer;

    convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

    physx::PxDefaultMemoryOutputStream buf;
    physx::PxConvexMeshCookingResult::Enum result;

    physx::PxTolerancesScale scale;
    physx::PxCookingParams params(scale);

    if (!PxCookConvexMesh(params, convexDesc, buf, &result))
        return NULL;
    physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
    physx::PxConvexMesh* convexMesh = physics->createConvexMesh(input);
    return convexMesh;
}

physx::PxTriangleMesh* PhysicsCooker::CookTriangleMesh(const float* vBuffer, uint32_t vCount, uint32_t vStride,
    const uint32_t* iBuffer, uint32_t iCount)
{
    auto* physics = Application::GetInstance().physics.get()->GetPhysics();

    physx::PxTriangleMeshDesc meshDesc;
    meshDesc.setToDefault();

    meshDesc.points.count = vCount;
    meshDesc.points.stride = vStride;
    meshDesc.points.data = vBuffer;

    meshDesc.triangles.count = iCount / 3;
    meshDesc.triangles.stride = 3 * sizeof(uint32_t);
    meshDesc.triangles.data = iBuffer;

    physx::PxTolerancesScale scale;
    physx::PxCookingParams params(scale);

    params.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eWELD_VERTICES;

    physx::PxDefaultMemoryOutputStream buf;
    physx::PxTriangleMeshCookingResult::Enum result;

    if (!PxCookTriangleMesh(params, meshDesc, buf, &result))
        return nullptr;

    physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
    physx::PxTriangleMesh* triangleMesh = physics->createTriangleMesh(input);

    return triangleMesh;
}