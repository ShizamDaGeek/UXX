#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "VAO.hpp"
#include "VBO.hpp"
#include "EBO.hpp"
#include "Shader.hpp"

struct Rect
{
    float xPos, yPos, width, height, rotation;

    Rect(float xPos, float yPos, float width, float height, float rotation)
        : xPos(xPos), yPos(yPos), width(width), height(height), rotation(rotation) {}
};
struct Color
{
    float r, g, b, a;

    Color(float r, float g, float b, float a)
        : r(r), g(g), b(b), a(a) {}
};
struct Style
{
    Color backgroundColor;
    Color borderColor;
    Color shadowColor;

    float borderWidth;

    float topleftCornerRadius;
    float topRightCornerRadius;
    float bottomLeftCornerRadius;
    float bottomRightCornerRadius;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void init();

    void drawUXXPanel(Rect rect, Color color);
    void drawUXXButton(Rect rect, Color color, Style style);
    void drawUXXSlider(Rect rect, Color color, Style style);
    void drawUXXSwitch(Rect rect, Color color, Style style);

    void blowup();
private:
    VAO* vao = nullptr;
    VBO* vbo = nullptr;
    EBO* ebo = nullptr;
    Shader* shader = nullptr;
};

#endif
