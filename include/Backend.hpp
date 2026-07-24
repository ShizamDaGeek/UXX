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

    bool mouseButtonDown[3] = {false, false, false};
    bool mouseButtonPressed[3] = {false, false, false};
    bool mouseButtonReleased[3] = {false, false, false};

    unsigned int SCREEN_WIDTH = 1920;
    unsigned int SCREEN_HEIGHT = 1080;

    double mouseX = 0.0, mouseY = 0.0;
    double scrollX = 0.0, scrollY = 0.0;

private:
    GLFWwindow* window;

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    void GetMouseInput();
};

#endif
