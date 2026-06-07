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
    void blowup();

private:
    GLFWwindow* window;
    Renderer* renderer = nullptr;
};

#endif
