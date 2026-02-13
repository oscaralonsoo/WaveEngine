////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_MESHDATA_H__
#define __GUI_MESHDATA_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsGui/Animatable.h>
#include <NsGui/IRenderProxyCreator.h>
#include <NsDrawing/Rect.h>
#include <NsCore/Vector.h>


namespace Noesis
{

struct Point;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents a low-level geometric mesh composed of vertices and indexed triangles.
///
/// MeshData allows direct submission of geometry to the GPU with no extra preprocessing. Each
/// vertex typically includes a position and UV coordinates, and triangles are defined via index
/// arrays. This makes the class ideal for rendering dynamic geometry efficiently, especially
/// when per-frame updates are required.
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API MeshData: public Animatable, public IRenderProxyCreator
{
public:
    MeshData();
    ~MeshData();

    /// Sets number of vertices
    void SetNumVertices(uint32_t numVertices);

    /// Sets number of texture coordinates
    void SetNumUVs(uint32_t numUVs);

    /// Sets number of indices
    void SetNumIndices(uint32_t numIndices);

    /// Sets bounding box
    void SetBounds(const Rect& bounds);

    /// Returns bounding box
    const Rect& GetBounds() const;

    /// Returns a pointer to the internal vertex buffer
    Point* GetVertices();

    /// Returns a pointer to the internal texture coordinates buffer
    Point* GetUVs();

    /// Returns a pointer to the internal index buffer
    uint16_t* GetIndices();

    enum Updated
    {
        Updated_Vertices = 1,
        Updated_UVs = 2,
        Updated_Indices = 4
    };

    /// Notify about a change in the geometry mesh
    void Updated(uint32_t flags);

    /// From Freezable
    //@{
    Ptr<MeshData> Clone() const;
    Ptr<MeshData> CloneCurrentValue() const;
    //@}

    /// From IRenderProxyCreator
    //@{
    void CreateRenderProxy(RenderTreeUpdater& updater, uint32_t proxyIndex) override;
    void UpdateRenderProxy(RenderTreeUpdater& updater, uint32_t proxyIndex) override;
    void UnregisterRenderer(ViewId viewId) override;
    //@}

    NS_IMPLEMENT_INTERFACE_FIXUP

protected:
    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    void CloneCommonCore(const Freezable* source) override;
    //@}

private:
    ProxyFlags mUpdateFlags;

    enum UpdateFlags
    {
        UpdateFlags_Vertices,
        UpdateFlags_UVs,
        UpdateFlags_Indices,
    };

    Vector<Point> mVertices;
    Vector<Point> mUVs;
    Vector<uint16_t> mIndices;

    Rect mBounds;

    NS_DECLARE_REFLECTION(MeshData, Animatable)
};

NS_WARNING_POP

}

#endif
