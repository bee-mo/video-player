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

    width_ = width;
    height_ = height;

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    gl_window_ = glfwCreateWindow(width, height, "Video Player", nullptr, nullptr);
    if (!gl_window_) {
        glfwTerminate();
        status = WINDOW_STATUS::FAILED_INITIALIZATION;
        return;
    }

    glfwMakeContextCurrent(gl_window_);
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        status = WINDOW_STATUS::FAILED_INITIALIZATION;
        return;
    }
    glfwSetInputMode(gl_window_, GLFW_STICKY_KEYS, GL_TRUE);

    const char *vertex_shader = 
    R"vert_shader(
        #version 330 core
        layout(location = 0) in vec3 vertexPosition_modelspace;
        layout(location = 1) in vec2 vertexUV;

        out vec2 UV;

        void main() {
            gl_Position.xyz = vertexPosition_modelspace;
            gl_Position.w = 1.0;

            UV = vertexUV;
        }
    )vert_shader";

    const char *fragment_shader =
    R"frag_shader(
        #version 330 core
        in vec2 UV;
        out vec3 color;
        uniform sampler2D tex_sampler;

        void main(){
            color = texture(tex_sampler, UV).rgb;
        }
    )frag_shader";

    shader_id_ = load_shaders(vertex_shader, fragment_shader);
    glUseProgram(shader_id_);
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    #define OFF_ 1.0f
    const GLfloat screen_rect[] = {
        -OFF_,  -OFF_,  0.0f,       0.0f,  0.0f,
        OFF_,   -OFF_,  0.0f,       1.0f,   0.0f,
        OFF_,   OFF_,   0.0f,       1.0f,   1.0f,
        OFF_,   OFF_,   0.0f,       1.0f,   1.0f,
        -OFF_,  OFF_,   0.0f,       0.0f,  1.0f,
        -OFF_,   -OFF_,   0.0f,     0.0f,  0.0f
    };

    // GLuint vbo;
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_rect), screen_rect, GL_STATIC_DRAW);

    pos_location_ = glGetAttribLocation(shader_id_, "vertexPosition_modelspace");
    glEnableVertexAttribArray(pos_location_);
    glVertexAttribPointer(pos_location_, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);

    img_location_ = glGetAttribLocation(shader_id_, "vertexUV");
    glEnableVertexAttribArray(img_location_);
    glVertexAttribPointer(img_location_, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));

    // generate random image
    std::vector<uint8_t> img;
    uint8_t byte_info;
    const float den = RAND_MAX == 0? 1.0f : RAND_MAX * 1.0f;
    for (int i = 0; i < width_ * height_ * 3; ++i) {
        byte_info = (uint8_t) (((rand() * 1.0f)/den) * 255);
        img.push_back(byte_info);
    }
    status = WINDOW_STATUS::OK;

    // draw_image(&img[0], width_, height_);

    /*
    GLuint image = window::generate_random_img(100, 100);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image);
    glUniform1i(glGetUniformLocation(shader_id_, "tex_sampler"), 0);

    // do a test draw
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_id_);
    glBindVertexArray(vao_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image);
    glEnableVertexAttribArray(pos_location);
    glEnableVertexAttribArray(img_location);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glfwSwapBuffers(gl_window_);
    */
    glfwPollEvents();
}

void window::draw_image(const uint8_t *img_buffer, int width, int height) {
    if (status != WINDOW_STATUS::OK) {
        fprintf(stderr, "Error: Window not initialized.\n");
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
        GL_UNSIGNED_BYTE, (const GLvoid *) img_buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(glGetUniformLocation(shader_id_, "tex_sampler"), 0);

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_id_);
    glBindVertexArray(vao_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glEnableVertexAttribArray(pos_location_);
    glEnableVertexAttribArray(img_location_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);

    glfwSwapBuffers(gl_window_);
    glfwPollEvents();
}

GLuint window::generate_random_img(int width, int height) {
    std::vector<float> img_buff;

    const float den = RAND_MAX == 0? 1.0f : RAND_MAX * 1.0f;
    for (int i = 0; i < width * height * 3; ++i) {
        img_buff.push_back((rand() * 1.0f)/den);
    }
    
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
        GL_FLOAT, (const GLvoid *) &img_buff[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return tex;
}

GLuint window::load_shaders(const char *vert_shader, const char *frag_shader) {
    
    GLint result {GL_FALSE};
    int info_log_len;
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertex_shader_id, 1, &vert_shader, nullptr);
    glCompileShader(vertex_shader_id);

    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_len);
	if ( info_log_len > 0 ){
		std::vector<char> VertexShaderErrorMessage(info_log_len+1);
		glGetShaderInfoLog(vertex_shader_id, info_log_len, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

    glShaderSource(fragment_shader_id, 1, &frag_shader, nullptr);
    glCompileShader(fragment_shader_id);

    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_len);
	if ( info_log_len > 0 ){
		std::vector<char> FragmentShaderErrorMessage(info_log_len+1);
		glGetShaderInfoLog(fragment_shader_id, info_log_len, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_len);
	if ( info_log_len > 0 ){
		std::vector<char> ProgramErrorMessage(info_log_len+1);
		glGetProgramInfoLog(program_id, info_log_len, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

    glDetachShader(program_id, vertex_shader_id);
    glDetachShader(program_id, fragment_shader_id);
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    return program_id;
}