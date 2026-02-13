////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_MESH_H__
#define __GUI_MESH_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsGui/FrameworkElement.h>


namespace Noesis
{

class MeshData;
class Brush;


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Renders a mesh geometry defined by vertices and indexed triangles.
///
/// The Mesh element acts as a container for MeshData, allowing you to draw custom geometric
/// shapes efficiently. Assign a MeshData instance to define the vertices, UVs, and triangle
/// indices that will be submitted directly to the GPU.
///
/// Use Mesh when you need precise control over low-level geometry, enabling high-performance,
/// dynamic rendering.
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API Mesh: public FrameworkElement
{
public:
    Mesh();
    ~Mesh();

    /// Gets or sets the MeshData to be rendered
    //@{
    MeshData* GetData() const;
    void SetData(MeshData* data);
    //@}

    /// Gets or sets the Brush used to fill the mesh
    //@{
    Brush* GetBrush() const;
    void SetBrush(Brush* brush);
    //@}

public:
    /// Dependency properties
    //@{
    static const DependencyProperty* DataProperty;
    static const DependencyProperty* BrushProperty;
    //@}

private:
    /// From UIElement
    //@{
    void OnRender(DrawingContext* context) override;
    //@}

    /// From FrameworkElement
    //@{
    Size MeasureOverride(const Size& availableSize) override;
    //@}

    NS_DECLARE_REFLECTION(Mesh, FrameworkElement)
};

}

#endif
