#pragma once

#include "gui.hpp"
#include "single-file-engine/Engine.hpp"
#include <vector>

struct Point
{
    float x;
    float y;
    int targetClass;
    Point(float x, float y, int targetClass) : x(x), y(y), targetClass(targetClass) {}
};

class Application
{
private:
    static constexpr int BATCH_SIZE = 32;
    std::vector<Point> points = {Point(0.5f, 0.5f, 1), Point(0.45, 0.3, 1), Point(-0.5, 0.5, 0), Point(-0.4, 0.55, 0)};
    Engine engine;

    DebugPanel debugPanel = DebugPanel([this]()
                                       { deleteLastPoint(); }, [this]()
                                       { clearPoints(); });

    size_t currentBatchStart = 0;
    bool shouldClose = false;

    bool mousePress = false;
    bool keyPress = false;

    void processInput()
    {
        glfwPollEvents();

        ImGuiIO &io = ImGui::GetIO();

        if (glfwGetMouseButton(engine.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !io.WantCaptureMouse)
        {
            mousePress = true;
        }
        else if (mousePress)
        {
            double xpos, ypos;
            glfwGetCursorPos(engine.window, &xpos, &ypos);
            int width = 0, height = 0;
            glfwGetFramebufferSize(engine.window, &width, &height);
            points.push_back(Point((xpos / width) * 2 - 1, (ypos / height) * 2 - 1, debugPanel.selectedClass));
            printf("%s\n", "click!");
            mousePress = false;
        }
        if (glfwGetKey(engine.window, GLFW_KEY_D) == GLFW_PRESS)
        {
            keyPress = true;
        }
        else if (keyPress)
        {
            deleteLastPoint();
            keyPress = false;
        }

        updatePoints();
    }
    void updateVisualization()
    {
    }
    void render()
    {
        engine.drawFrame();
    }

public:
    Application()
    {
        engine = Engine();
    }
    void init()
    {
        engine.initWindow();
        // glfwSetKeyCallback(engine.window, keyCallback);
        engine.initVulkan();

        engine.pushLayer(&debugPanel);
    }

    void deleteLastPoint()
    {
        points.pop_back();
    }
    void clearPoints()
    {
        points.clear();
    }

    void updatePoints()
    {
        engine.figures.clear();
        for (auto &p : points)
        {
            if (p.targetClass == 0)
                engine.figures.push_back(Circle(p.x, p.y, 0.05, 1, 0, 0));
            if (p.targetClass == 1)
                engine.figures.push_back(Circle(p.x, p.y, 0.05, 0, 1, 0));
        };
    }

    void mainLoop()
    {
        updatePoints();
        while (!glfwWindowShouldClose(engine.window))
        {
            processInput();

            // if (!points.empty())
            // {
            //     std::vector<Point> batch = getNextBatch(BATCH_SIZE);
            //     // network.trainBatch(batch, learningRate);
            // }

            updateVisualization();
            render();
        }

        engine.cleanup();

        getchar();
    }

    std::vector<Point> getNextBatch(int size)
    {
        std::vector<Point> batch;
        for (int i = 0; i < size; ++i)
        {
            batch.push_back(points[currentBatchStart]);
            currentBatchStart = (currentBatchStart + 1) % points.size();
        }
        return batch;
    }
};
