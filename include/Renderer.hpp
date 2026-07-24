#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <iostream>
#include <string>
#include <unordered_set>
#include <unordered_map>

#include "Texture.hpp"
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

namespace Renderer
{
    inline float SCREEN_WIDTH = 1920.0f;
    inline float SCREEN_HEIGHT = 1080.0f;
    inline float SCREEN_SCALE_X = 1.0f;
    inline float SCREEN_SCALE_Y = 1.0f;

    void init();

    void SetMouseState(double x, double y, bool leftDown, bool rightDown, bool middleDown);

    void BeginPanel(Rect PanelRect, Color PanelColor);
    void EndPanel();

    bool DrawButton(Rect ButtonRect, Color ButtonColor, std::string ButtonImagePath);
    void DrawSlider(Rect SliderRect, Color SliderColor);
    void DrawSwitch(Rect SwitchRect, Color SwitchColor);

    void DrawImage(Rect ImageRect, Color ImageColor, std::string ImagePath);

    void BlowUp();
};

namespace UXX = Renderer;

#endif
