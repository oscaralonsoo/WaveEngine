////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_VIRTUALIZINGWRAPPANEL_H__
#define __GUI_VIRTUALIZINGWRAPPANEL_H__


#include <NsCore/Noesis.h>
#include <NsGui/VirtualizingPanel.h>
#include <NsGui/IScrollInfo.h>


namespace Noesis
{

class ItemsControl;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// A high-performance WrapPanel that supports UI virtualization, designed for use in both
/// horizontal and vertical layouts.
///
/// This panel arranges its child elements sequentially in a wrapping fashion, either horizontally
/// or vertically, and only creates UI elements that are visible within the viewport, reducing
/// memory usage and improving rendering performance for large collections.
///
/// .. code-block:: xml
///
///  <Grid
///    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
///    <ItemsControl ItemsSource="{Binding List}"
///        ScrollViewer.CanContentScroll="True"
///        VirtualizingPanel.IsVirtualizing="True"
///        VirtualizingPanel.ScrollUnit="Pixel">
///      <ItemsControl.Template>
///        <ControlTemplate TargetType="ItemsControl">
///          <ScrollViewer Focusable="False" Background="Silver">
///            <ItemsPresenter/>
///          </ScrollViewer>
///        </ControlTemplate>
///      </ItemsControl.Template>
///      <ItemsControl.ItemsPanel>
///        <ItemsPanelTemplate>
///          <VirtualizingWrapPanel Orientation="Horizontal"/>
///        </ItemsPanelTemplate>
///      </ItemsControl.ItemsPanel>
///      <ItemsControl.ItemTemplate>
///        <DataTemplate>
///          <Rectangle Stroke="Black" Fill="{Binding Color}"
///              Width="{Binding SizeX}" Height="{Binding SizeY}"/>
///        </DataTemplate>
///      </ItemsControl.ItemTemplate>
///    </ItemsControl>
///  </Grid>
///
/// .. image:: VirtualizingWrapPanel0.png
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API VirtualizingWrapPanel: public VirtualizingPanel, public IScrollInfo
{
public:
    VirtualizingWrapPanel();
    ~VirtualizingWrapPanel();

    /// Gets or sets a value that specifies the width of all items contained in the panel
    //@{
    float GetItemWidth() const;
    void SetItemWidth(float itemWidth);
    //@}

    /// Gets or sets a value that specifies the height of all items contained in the panel
    //@{
    float GetItemHeight() const;
    void SetItemHeight(float itemHeight);
    //@}

    /// Gets or sets orientation of children elements
    //@{
    Orientation GetOrientation() const;
    void SetOrientation(Orientation orientation);
    //@}

    // From IScrollInfo
    //@{
    bool GetCanHorizontallyScroll() const override;
    void SetCanHorizontallyScroll(bool canScroll) override;
    bool GetCanVerticallyScroll() const override;
    void SetCanVerticallyScroll(bool canScroll) override;
    float GetExtentWidth() const override;
    float GetExtentHeight() const override;
    float GetViewportWidth() const override;
    float GetViewportHeight() const override;
    float GetHorizontalOffset() const override;
    float GetVerticalOffset() const override;
    ScrollViewer* GetScrollOwner() const override;
    void SetScrollOwner(ScrollViewer* owner) override;
    void LineLeft() override;
    void LineRight() override;
    void LineUp() override;
    void LineDown() override;
    void PageLeft() override;
    void PageRight() override;
    void PageUp() override;
    void PageDown() override;
    void MouseWheelLeft(float delta = 1.0f) override;
    void MouseWheelRight(float delta = 1.0f) override;
    void MouseWheelUp(float delta = 1.0f) override;
    void MouseWheelDown(float delta = 1.0f) override;
    void SetHorizontalOffset(float offset) override;
    void SetVerticalOffset(float offset) override;
    Rect MakeVisible(Visual* visual, const Rect& rect) override;
    //@}

    NS_IMPLEMENT_INTERFACE_FIXUP

public:
    /// Dependency properties
    //@{
    static const DependencyProperty* ItemWidthProperty;
    static const DependencyProperty* ItemHeightProperty;
    static const DependencyProperty* OrientationProperty;
    //@}

protected:
    /// From FrameworkElement
    //@{
    Size MeasureOverride(const Size& availableSize) override;
    Size ArrangeOverride(const Size& finalSize) override;
    //@}

    // From Panel
    //@{
    void OnItemsChangedOverride(BaseComponent* sender, const ItemsChangedEventArgs& e) override;
    void GenerateChildren() override;
    void OnConnectToGenerator(ItemsControl* itemsControl) override;
    void OnDisconnectFromGenerator() override;
    //@}

    // From VirtualizingPanel
    //@{
    void BringIndexIntoViewOverride(int32_t index) override;
    //@}

private:
    void MakeOffsetVisible(float& offset, float& p0, float& p1, float viewport);

    void OffsetToIndex(Point& offset, bool itemScrolling, bool isHorizontal);
    float OffsetToIndex(ItemContainerGenerator* generator, UIElementCollection* children,
        int numItems, const Size& constraint, const Size& childConstraint,
        bool isHorizontal, bool itemScrolling, bool isRecycling, float& direction);
    int CachedItems(int viewportSections, float cacheLength,
        VirtualizationCacheLengthUnit cacheUnit);
    void EnsureEstimatedSize(ItemContainerGenerator* generator, UIElementCollection* children,
        int numItems, const Size& constraint, const Size& childConstraint,
        bool isHorizontal, bool isRecycling);
    void UpdateSectionsInfo(int start, int end, int numItems);
    void UpdateSectionSize(int index, float sectionSize, int start, int end, int numItems);
    float SectionSize(int i) const;
    float SectionSize(float size) const;

    Size MeasureViewport(ItemContainerGenerator* generator, UIElementCollection* children,
        const Size& constraint, float viewportSize, int numItems,
        float itemW, float itemH, bool autoW, bool autoH, const Size& childConstraint,
        int& firstVisible, int& lastVisible, float& accumVisibleSize, bool isHorizontal,
        bool itemScrolling, bool isRecycling);
    Size MeasureSection(ItemContainerGenerator* generator, UIElementCollection* children,
        const Size& constraint, const Size& childConstraint,
        float itemW, float itemH, bool autoW, bool autoH, int start, int end, int direction,
        bool isHorizontal, bool isRecycling, int& last,
        bool& visibleContainerMeasured, int& visibleContainerIndex);
    void MeasureUnrealized(ItemContainerGenerator* generator, const Size& childConstraint,
        Size& desiredSize, int start, int end, int startItems, int endItems,
        bool isHorizontal, bool updateArrangeOffset = false);
    void AccumDeltaSize(float oldSize, float newSize, float& deltaSize, bool shouldAccumSize = true);
    bool AdjustOffset(float direction, float viewportSize, float& deltaSize,
        float& accumVisibleSize, Size& desiredSize, float firstVisibleSize,
        float firstVisiblePerc, int& firstVisible, int& lastVisible, int sectionIndex,
        bool isHorizontal, bool itemScrolling, bool makingVisible,
        bool visibleContainerMeasured, int visibleContainerIndex);
    void AdjustOffset(float deltaSize, bool isHorizontal);

    int FindSection(int sectionIndex, int itemIndex) const;

    Point ArrangeOffset(bool isHorizontal, bool itemScrolling);
    void AccumArrangeOffset(const Point& baseOffset, Point& offset, int start, int end,
        float averageSize, bool isHorizontal);

    Ptr<UIElement> GenerateContainer(ItemContainerGenerator* generator,
        UIElementCollection* children, int index, bool isRecycling);
    Ptr<UIElement> GenerateContainer(ItemContainerGenerator* generator,
        UIElementCollection* children, int index, bool isRecycling, bool& isNewChild);
    bool IsRecycledContainer(DependencyObject* container, UIElementCollection* children,
        int index, bool isRecycling);
    void GenerateRange(ItemContainerGenerator* generator, UIElementCollection* children,
        const Size& constraint, const Size& childConstraint, Size& desiredSize,
        float itemW, float itemH, bool autoW, bool autoH,
        int start, int count, int itemIndex, int numItems, bool isHorizontal, bool isRecycling);

    void RemoveHiddenItems(ItemsControl* itemsControl, ItemContainerGenerator* generator,
        UIElementCollection* children, int firstVisibleItem, int lastVisibleItem, bool isRecycling);
    void RemoveRange(ItemContainerGenerator* generator, UIElementCollection* children,
        int start, int count, bool isRecycling);

    void UpdateScrollData(bool isHorizontal, bool itemScrolling,
        const Size& extent, int itemExtent,
        const Size& viewport, int itemViewport,
        const Point& offset, int itemOffset);

    void EnsureScrollData();
    bool IsScrolling() const;
    bool IsHorizontal() const;
    bool IsVertical() const;
    bool ItemScrolling() const;
    bool ItemScrolling(ItemsControl* itemsControl) const;

    void ResetScrolling();

    void CheckVirtualization(BaseComponent* sender, const RoutedEventArgs& e);

private:
    struct ScrollData;
    ScrollData* mScrollData;

    NS_DECLARE_REFLECTION(VirtualizingWrapPanel, VirtualizingPanel)
};

NS_WARNING_POP

}


#endif
