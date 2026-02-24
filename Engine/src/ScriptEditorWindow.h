#pragma once

#include "EditorWindow.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <unordered_set>

#include "TextEditor.h" 

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

enum class ScriptTokenType
{
    NONE,
    KEYWORD,
    FUNCTION,
    STRING,
    NUMBER,
    COMMENT,
    OPERATOR,
    IDENTIFIER
};

struct SyntaxError
{
    int lineNumber = 0;
    std::string message;
};

struct TextLine
{
    std::string content;
    bool hasError = false;
    std::string errorMessage;
};

// Structure for each open tab/script   
struct ScriptTab
{
    std::string filePath;
    std::string fileName;         
    std::string originalContent;
    char* textBuffer = nullptr;
    bool hasUnsavedChanges = false;
    std::vector<TextLine> lines;
    std::vector<SyntaxError> syntaxErrors;
    float scrollY = 0.0f;
    bool wantClose = false;    

    ScriptTab() = default;
    ~ScriptTab() {
        if (textBuffer) {
            delete[] textBuffer;
            textBuffer = nullptr;
        }
    }

    ScriptTab(ScriptTab&& other) noexcept
        : filePath(std::move(other.filePath))
        , fileName(std::move(other.fileName))
        , originalContent(std::move(other.originalContent))
        , textBuffer(other.textBuffer)
        , hasUnsavedChanges(other.hasUnsavedChanges)
        , lines(std::move(other.lines))
        , syntaxErrors(std::move(other.syntaxErrors))
        , scrollY(other.scrollY)
        , wantClose(other.wantClose)
    {
        other.textBuffer = nullptr;
    }

    ScriptTab& operator=(ScriptTab&& other) noexcept {
        if (this != &other) {
            if (textBuffer) delete[] textBuffer;
            filePath = std::move(other.filePath);
            fileName = std::move(other.fileName);
            originalContent = std::move(other.originalContent);
            textBuffer = other.textBuffer;
            hasUnsavedChanges = other.hasUnsavedChanges;
            lines = std::move(other.lines);
            syntaxErrors = std::move(other.syntaxErrors);
            scrollY = other.scrollY;
            wantClose = other.wantClose;
            other.textBuffer = nullptr;
        }
        return *this;
    }

    // Disable copy 
    ScriptTab(const ScriptTab&) = delete;
    ScriptTab& operator=(const ScriptTab&) = delete;
};

class ScriptEditorWindow : public EditorWindow
{
public:
    ScriptEditorWindow();
    ~ScriptEditorWindow();

    void Draw() override;

    // Open a script for editing
    void OpenScript(const std::string& scriptPath);

    // Save current script
    bool SaveScript();

    // Save a specific tab
    bool SaveTab(int tabIndex);

    // Close a specific tab
    void CloseTab(int tabIndex);

    // Check if there are unsaved changes in any tab
    bool HasUnsavedChanges() const;

    // Get number of open tabs
    size_t GetOpenTabCount() const { return openTabs.size(); }

private:
    void DrawMenuBar();
    void DrawTabs();
    void DrawTextEditor();
    void DrawColoredReadOnlyView(ImVec2 editorSize, float lineNumberWidth);
    void DrawEditableView(ImVec2 editorSize, float lineNumberWidth);
    void DrawLineNumbers();
    void DrawErrorPanel();
    void DrawStatusBar();

    bool LoadScriptIntoTab(ScriptTab& tab, const std::string& path);
    void MarkCurrentTabAsModified();

    //Script text error
    int ExtractLineFromError(const std::string& error);
    bool showErrorPopup = false;
    std::string errorMessage;

    // Syntax highlighting
    void InitializeLuaKeywords();
    ImVec4 GetColorForToken(ScriptTokenType type);
    void RenderTextWithSyntaxHighlighting(const std::string& text, int lineNumber, ImVec2 startPos);
    ScriptTokenType IdentifyToken(const std::string& token);
    bool IsLuaKeyword(const std::string& word);
    bool IsLuaFunction(const std::string& word);

    // Error checking
    void CheckSyntaxErrors();
    void CheckSyntaxErrorsForTab(ScriptTab& tab);
    void ParseTextIntoLines();
    void ParseTextIntoLinesForTab(ScriptTab& tab);

    // Helpers
    int FindTabByPath(const std::string& path) const;
    ScriptTab* GetActiveTab();

	// Tab management
    std::vector<ScriptTab> openTabs;
    int activeTabIndex = -1;
	size_t bufferSize = 1024 * 1024;  // 1MB for buffer

    // Editor settings
    TextEditor editor;

    bool showLineNumbers = true;
    float fontSize = 16.0f;
    bool enableSyntaxHighlighting = true;
    bool autoCheckSyntax = true;

    // Search/Replace
    char searchText[256] = {};
    char replaceText[256] = {};
    bool showSearchReplace = false;

    // Syntax highlighting data
    std::unordered_set<std::string> luaKeywords;
    std::unordered_set<std::string> luaBuiltinFunctions;

    // Colors
    ImVec4 colorKeyword;
    ImVec4 colorFunction;
    ImVec4 colorString;
    ImVec4 colorNumber;
    ImVec4 colorComment;
    ImVec4 colorOperator;
    ImVec4 colorIdentifier;
    ImVec4 colorError;
    ImVec4 colorLineNumber;
    ImVec4 colorBackground;

	// Tab pending close index
    int tabPendingClose = -1;
};
