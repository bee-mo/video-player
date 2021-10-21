#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <cstdint>
#include <cstdio>
#include <vector>
#include <cstdlib>

#include <GL/glew.h>
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

    /**
     * @def
     * Draw the image in [img_buffer] to the screen with opengl.
     * @param img_buffer The image buffer to draw onto the screen. It is
     * in the format RGB (every 3 bytes represnet red, green and blue color
     * respectively for one pixel).
     * @param width The width of the image represented by the image buffer
     * @param height The height of the image represented by the image buffer
     * */
    void draw_image(const uint8_t *img_buffer, int width, int height);

private:
    GLFWwindow *gl_window_;
    WINDOW_STATUS status;
    GLuint shader_id_;
    GLuint vao_;

    /**
     * @def
     * Create a shader program with the given vertex & fragment
     * shader.
     * @returns the id of the shader program that was created.
     *  If an error occurs, then an error value is returned.
     * */
    static GLuint load_shaders(const char *vert_shader, const char *frag_shader);
    static GLuint generate_random_img(int width, int height);
};

#endif