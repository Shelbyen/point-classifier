#pragma once

#include "Application.hpp"


int main() {
    Application app = Application();
    app.init();
    app.mainLoop();
}
