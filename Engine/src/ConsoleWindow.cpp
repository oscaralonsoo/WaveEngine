#include "ConsoleWindow.h"
#include <imgui.h>
#include "Log.h"

ConsoleWindow::ConsoleWindow()
    : EditorWindow("Console")
{
}

void ConsoleWindow::Draw()
{
    if (!isOpen) return;

    ImGui::Begin(name.c_str(), &isOpen);

    if (ImGui::Button("Clear"))
    {
        ConsoleLog::GetInstance().Clear();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Library Info", &showLibraryInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Errors", &showErrors);
    ImGui::SameLine();
    ImGui::Checkbox("Warnings", &showWarnings);
    ImGui::SameLine();
    ImGui::Checkbox("Loading", &showLoading);
    ImGui::SameLine();
    ImGui::Checkbox("Success", &showSuccess);
    ImGui::SameLine();
    ImGui::Checkbox("Info", &showInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::Separator();

    ImVec2 availableSpace = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("Scrolling", availableSpace, true, ImGuiWindowFlags_HorizontalScrollbar);

    const std::vector<std::string>& logs = ConsoleLog::GetInstance().GetLogs();

    for (const auto& log : logs)
    {
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        bool isError = false;
        bool isWarning = false;
        bool isInfo = false;
        bool isSuccess = false;
        bool isLoading = false;
        bool isLibraryInfo = false;

        if (log.find("DevIL") != std::string::npos ||
            log.find("SDL3") != std::string::npos ||
            log.find("OpenGL") != std::string::npos ||
            log.find("ASSIMP") != std::string::npos ||
            log.find("Mesh processed") != std::string::npos)
        {
            color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            isLibraryInfo = true;
        }
        else if (log.find("ERROR") != std::string::npos || log.find("Failed") != std::string::npos)
        {
            color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
            isError = true;
        }
        else if (log.find("WARNING") != std::string::npos || log.find("Corrupt") != std::string::npos)
        {
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            isWarning = true;
        }
        else if (log.find("Loading") != std::string::npos || log.find("Initializing") != std::string::npos)
        {
            color = ImVec4(1.0f, 0.7f, 0.3f, 1.0f);
            isLoading = true;
        }
        else if (log.find("ready") != std::string::npos ||
            log.find("loaded") != std::string::npos ||
            log.find("initialized") != std::string::npos)
        {
            color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
            isSuccess = true;
        }
        else
            isInfo = true;

        if (isLibraryInfo)
        {
            if (!showLibraryInfo)
                continue;
        }
        else if ((isError && !showErrors) ||
            (isWarning && !showWarnings) ||
            (isSuccess && !showSuccess) ||
            (isInfo && !showInfo) ||
            (isLoading && !showLoading))
        {
            continue;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextWrapped("%s", log.c_str());
        ImGui::PopStyleColor();
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    if (scrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
    }

    ImGui::EndChild();
    ImGui::End();
}