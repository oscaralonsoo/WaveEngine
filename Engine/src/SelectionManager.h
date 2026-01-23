#pragma once
#include <vector>
#include <algorithm>

class GameObject;

class SelectionManager
{
public:

	static SelectionManager& GetInstance() {
			static SelectionManager instance;
			return instance;
		}
	SelectionManager() = default;
	~SelectionManager() = default;

	// Single selection (replaces current selection)
	void SetSelectedObject(GameObject* obj);

	// Multi-selection operations
	void AddToSelection(GameObject* obj);
	void RemoveFromSelection(GameObject* obj);
	void ToggleSelection(GameObject* obj);

	// Clear all selections
	void ClearSelection();

	// Query methods
	GameObject* GetSelectedObject() const;
	const std::vector<GameObject*>& GetSelectedObjects() const { return selectedObjects; }
	bool IsSelected(GameObject* obj) const;
	bool HasSelection() const { return !selectedObjects.empty(); }
	int GetSelectionCount() const { return static_cast<int>(selectedObjects.size()); }

private:
	std::vector<GameObject*> selectedObjects;
};