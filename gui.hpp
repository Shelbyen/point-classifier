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
    float learningRate = 0.1f;
    int epoch = 5;

    float f1Score = 0.0f;
    float avgLoss = 0.0f;

    void onGui() override
    {
        ImGui::Begin("Debug");
        ImGui::RadioButton("Class a", &selectedClass, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Class b", &selectedClass, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Class c", &selectedClass, 2);

        if (ImGui::Button("Delete last"))
            deleteLastPoint();

        if (ImGui::Button("Delete All"))
            clearPoints();

        ImGui::SliderFloat("Learning rate", &learningRate, 0, 0.1);
        ImGui::SliderInt("Epoch per frame", &epoch, -15, 50);

        ImGui::Separator();
        ImGui::Text("F1 Score:    %.4f", f1Score);
        ImGui::Text("Avg Loss:    %.4f", avgLoss);

        ImGui::End();
    }

private:
    std::function<void()> deleteLastPoint;
    std::function<void()> clearPoints;
};
