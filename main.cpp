#pragma once

#include "Application.hpp"


int main() {
    Application app = Application("dataset1.csv");
    app.init();
    app.mainLoop();
}
