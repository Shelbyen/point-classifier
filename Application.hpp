#pragma once

#include <vector>


class Point {
    // x, y, target_class
};


class Application {
private:
    static constexpr int BATCH_SIZE = 32;
    std::vector<Point> points;
    size_t currentBatchStart = 0;
    bool shouldClose = true;    // CHANGE IF DONE WITH LOOP

void processInput();
void updateVisualization();
void render();

public:
    void mainLoop() {
        while (!shouldClose) {
            processInput();
            
            if (!points.empty()) {
                std::vector<Point> batch = getNextBatch(BATCH_SIZE);
                // network.trainBatch(batch, learningRate);
            }
            
            updateVisualization();
            render();
        }
    }
    
    std::vector<Point> getNextBatch(int size) {
        std::vector<Point> batch;
        for (int i = 0; i < size; ++i) {
            batch.push_back(points[currentBatchStart]);
            currentBatchStart = (currentBatchStart + 1) % points.size();
        }
        return batch;
    }
};
