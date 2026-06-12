#pragma once

#include <functional>
#include "single-file-engine/Engine.hpp"


class DebugPanel : public IGuiLayer
{
public:
    // TODO: Remove app
    DebugPanel(std::function<void()> d, std::function<void()> c) : deleteLastPoint(d), clearPoints(c)
    {
    }
    int selectedClass = 0;
    void onGui() override
    {
        ImGui::Begin("Debug");
        ImGui::RadioButton("Class a", &selectedClass, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Class b", &selectedClass, 1);

        if (ImGui::Button("Delete last"))
            deleteLastPoint();

        if (ImGui::Button("Delete All"))
            clearPoints();

        ImGui::End();
    }

private:
    std::function<void()> deleteLastPoint;
    std::function<void()> clearPoints;
};
