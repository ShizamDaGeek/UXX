#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Renderer
{
public:
        Renderer();
        ~Renderer();

        void renderTriangle();
};

#endif
