#include "window.h"

window::~window() {
    if (status == WINDOW_STATUS::OK) {
        glfwTerminate();
    }
}

void window::init(int width, int height) {
    if (!glfwInit()) {
        status = WINDOW_STATUS::FAILED_INITIALIZATION;
        return;
    }

    gl_window_ = glfwCreateWindow(width, height, "Video Player", nullptr, nullptr);
    if (!gl_window_) {
        glfwTerminate();
        status = WINDOW_STATUS::FAILED_INITIALIZATION;
        return;
    }

    glfwMakeContextCurrent(gl_window_);
    status = WINDOW_STATUS::OK;
}
