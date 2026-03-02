#include "MeshRenderer.h"

class SkinnedMeshRenderer : public MeshRenderer
{
public:
    SkinnedMeshRenderer(GameObject* owner);

    void Update() override;
    void CleanUp() override;

    ComponentType GetType() override { return ComponentType::SkinnedMeshRenderer; }
    bool IsType(ComponentType type) override { return type == ComponentType::SkinnedMeshRenderer || type == ComponentType::MeshRenderer;};
    bool IsIncompatible(ComponentType type) override { return type == ComponentType::SkinnedMeshRenderer || type == ComponentType::MeshRenderer;};

    AABB GetGlobalAABB() override;

    void SetMeshResource(UID uid) override;

    void LinkBones();
    void UpdateSkinningMatrices();
    void UpdateDynamicAABB();
    bool HasSkinning() const override { return hasSkinningData; }

    const glm::mat4& GetMeshInverse() const { return meshInverseTransform; }
    unsigned int GetSSBOGlobal() const { return ssboGlobalMatrices; }
    unsigned int GetSSBOOffset() const { return ssboOffsetMatrices; }

    //void OnEvent(const Event& event) override;

    void OnEditor() override;

private:
    struct BoneInfo {
        const glm::mat4* globalMatrixPtr;
        glm::mat4 offsetMatrix;
    };

    std::vector<BoneInfo> fastBones;
    std::vector<GameObject*> boneGameObjects;
    std::vector<glm::mat4> cachedBoneMatrices;
    std::vector<glm::mat4> boneGlobalMatrices;
    glm::mat4 meshInverseTransform;




    unsigned int ssboGlobalMatrices = 0;
    unsigned int ssboOffsetMatrices = 0;

    bool bonesLinked = false;
    bool hasSkinningData = false;
    AABB dynamicAABB;
};