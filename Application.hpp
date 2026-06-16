#pragma once

#include "gui.hpp"
#include "single-file-engine/Engine.hpp"
#include <vector>
#include "machine-learning-lib/NeuralNet.h"
#include "utils.hpp"

struct Point
{
    float x;
    float y;
    int targetClass;
    Point(float x, float y, int targetClass) : x(x), y(y), targetClass(targetClass) {}
};

struct BgUniforms
{
    float time;
    float resX;
    float resY;
    float _pad;
};

static const int ARCHITECTURE[] = {2, 5, 5, 3};  // input, hidden..., output
static const int ARCH_SIZE = sizeof(ARCHITECTURE) / sizeof(ARCHITECTURE[0]);

static const int WEIGHTS_COUNT = []() {
    int total = 0;
    for (int i = 0; i < ARCH_SIZE - 1; i++)
        total += ARCHITECTURE[i] * ARCHITECTURE[i+1] + ARCHITECTURE[i+1];
    return total;
}();


struct BgPushConstants
{
    int layer_count;
    int input_size;
    int output_size;
    int hidden_sizes[8];
    float resX;
    float resY;
};

struct BgState
{
    BgPushConstants push;
    float *weights;
    void *mapped;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDescriptorSet descriptorSet;
    Engine *engine;
};

static BgState bgState = {};

static void bgSetup(VkDescriptorSetLayout *outLayout, uint32_t *outPushConstantSize, void *userdata)
{
    BgState *s = (BgState *)userdata;

    s->weights = (float *)malloc(WEIGHTS_COUNT * sizeof(float));
    for (int i = 0; i < WEIGHTS_COUNT; i++)
        s->weights[i] = ((float)rand() / RAND_MAX) * 0.4f - 0.1f;

    *outPushConstantSize = sizeof(BgPushConstants);

    s->push.layer_count = ARCH_SIZE - 1;
    s->push.input_size  = ARCHITECTURE[0];
    s->push.output_size = ARCHITECTURE[ARCH_SIZE - 1];
    for (int i = 0; i < ARCH_SIZE - 2; i++)
        s->push.hidden_sizes[i] = ARCHITECTURE[i + 1];

    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCI = {};
    layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCI.bindingCount = 1;
    layoutCI.pBindings = &binding;
    s->engine->createDescriptorSetLayout(layoutCI, outLayout);

    s->descriptorSet = s->engine->createSSBO(
        WEIGHTS_COUNT * sizeof(float), *outLayout,
        &s->mapped, &s->buffer, &s->memory);

    memcpy(s->mapped, s->weights, WEIGHTS_COUNT * sizeof(float));
}

static void bgBind(VkCommandBuffer cmd, VkPipelineLayout layout, void *userdata)
{
    BgState *s = (BgState *)userdata;

    vkCmdPushConstants(cmd, layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(BgPushConstants), &s->push);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            layout, 0, 1, &s->descriptorSet, 0, NULL);
}

static void bgUpdate(void *userdata)
{
    BgState* s = (BgState*)userdata;
    memcpy(s->mapped, s->weights, WEIGHTS_COUNT * sizeof(float));

    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = s->memory;
    range.offset = 0;
    range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(s->engine->getDevice(), 1, &range);

    s->push.resX = (float)s->engine->getExtent().width;
    s->push.resY = (float)s->engine->getExtent().height;
}

class Application
{
private:
    static constexpr int BATCH_SIZE = 32;
    std::vector<Point> points = {    Point(0.2f, 0.2f, 1),
    Point(0.8f, 0.8f, 1),
    Point(0.8f, 0.2f, 0),
    Point(0.2f, 0.8f, 0), };
    Engine engine;

    DebugPanel debugPanel = DebugPanel([this]()
                                       { deleteLastPoint(); }, [this]()
                                       { clearPoints(); });

    size_t currentBatchStart = 0;
    bool shouldClose = false;

    bool mousePress = false;
    bool keyPress = false;

    NeuralNet net;

    std::vector<TrainingSample> samples;

    int frameEpochCounter = 0;

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
            points.push_back(Point((xpos / width), (ypos / height), debugPanel.selectedClass));
            printf("%s\n", "click!");
            mousePress = false;

            updatePoints();
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

        net.setLearningRate(debugPanel.learningRate);
    }

    void updateVisualization()
    {
        size_t n = net.weightsCount();

        net.exportWeights(bgState.weights);
    }
    void render()
    {
        engine.drawFrame();
    }

    void train(const std::vector<TrainingSample>& batch, double learningRate, size_t epochs)
    {
        for (const auto& sample : batch)
        {
            net.train(sample.input, sample.target);
        }
    }

public:
    Application()
    {
        engine = Engine();
    }
    void init()
    {
        engine.initWindow();
        bgState.engine = &engine;

        engine.setBgCallbacks(bgSetup, bgBind, bgUpdate, &bgState);

        engine.initVulkan();

        engine.pushLayer(&debugPanel);
        initNet();

    }

    void initNet()
    {
        for (int i = 0; i < ARCH_SIZE - 1; i++)
        {
            Layer l = Layer(ARCHITECTURE[i], ARCHITECTURE[i + 1]);
            l.setActivation(i == ARCH_SIZE - 2 ? ActivationType::Sigmoid : ActivationType::ReLU);
            net.addLayer(l);
        }

        net.setLearningRate(debugPanel.learningRate);
    }

    void deleteLastPoint()
    {
        points.pop_back();
        samples.pop_back();
        updatePoints();
    }
    void clearPoints()
    {
        points.clear();
        samples.clear();
        updatePoints();
    }

    void updatePoints()
    {
        engine.figures.clear();
        for (auto &p : points)
        {
            TrainingSample s;
            s.input = Tensor(std::vector<double>{p.x, p.y});

            switch (p.targetClass) {
                case 0: {
                    engine.figures.push_back(Circle(p.x, p.y, 0.05, 1, 0, 0));
                    s.target = Tensor(std::vector<double>{1.0, 0.0, 0.0});
                    break;
                }
                case 1: {
                    engine.figures.push_back(Circle(p.x, p.y, 0.05, 0, 1, 0));
                    s.target = Tensor(std::vector<double>{0.0, 1.0, 0.0});
                    break;
                }
                case 2: {
                    engine.figures.push_back(Circle(p.x, p.y, 0.05, 0, 0, 1));
                    s.target = Tensor(std::vector<double>{0.0, 0.0, 1.0});
                    break;
                }
            }
            samples.push_back(s);
        };
    }

    void mainLoop()
    {
        updatePoints();
        while (!glfwWindowShouldClose(engine.window))
        {
            processInput();

            if (!points.empty())
            {
                if (debugPanel.epoch < 0 && debugPanel.epoch + frameEpochCounter == 0 || debugPanel.epoch == 0)
                {
                    TrainingSample r = *select_randomly(samples.begin(), samples.end());
                    net.train(r.input, r.target);
                    frameEpochCounter = 0;
                } else if (debugPanel.epoch < 0)
                {
                    frameEpochCounter++;
                } else
                {
                    for (int i = 0; i < debugPanel.epoch; i++) {
                        TrainingSample r = *select_randomly(samples.begin(), samples.end());
                        net.train(r.input, r.target);
                    }
                }
                

                // Tensor out = net.predict(Tensor(std::vector<double>{0.5, 0.5}));
                // out.print();
            }

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
