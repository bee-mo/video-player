#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <GLFW/glfw3.h>

enum WINDOW_STATUS 
{
    OK,
    FAILED_INITIALIZATION
};

class window {

public:
    window() = default;
    window(const window &w) = delete;
    ~window();
    
    void operator=(const window &w) = delete;

    void init(int width, int height);

private:
    GLFWwindow *gl_window_;
    WINDOW_STATUS status;
};

#endif