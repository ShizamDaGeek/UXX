#ifndef BACKEND_HPP
#define BACKEND_HPP

// The Backend will take care of the window making and initialization/clearing libraries

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>

#include "Renderer.hpp"
#include "Shader.hpp"
#include "VAO.hpp"
#include "VBO.hpp"
#include "EBO.hpp"

class Backend
{
public:
    Backend();
    ~Backend();

    bool init();
    void run();
    void die();

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    unsigned int SCREEN_WIDTH = 1920;
    unsigned int SCREEN_HEIGHT = 1080;

private:
    GLFWwindow* window;

    void GetMouseInput();
};

#endif
