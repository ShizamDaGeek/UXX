#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <iostream>
#include <string>
#include <unordered_set>

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

class Renderer
{
public:
    float SCREEN_WIDTH = 1920.0f;
    float SCREEN_HEIGHT = 1080.0f;

    Renderer();
    ~Renderer();

    void init();

    void BeginUXXPanel(Rect PanelRect, Color PanelColor);
    void EndUXXPanel();

    void DrawUXXButton(Rect ButtonRect, Color ButtonColor, std::string ButtonImagePath);
    void DrawUXXSlider(Rect SliderRect, Color SliderColor);
    void DrawUXXSwitch(Rect SwitchRect, Color SwitchColor);

    void DrawUXXImage(Rect ImageRect, Color ImageColor, std::string ImagePath);

    void BlowUp();
private:
    bool panelOpen = false;

    VAO* vao = nullptr;
    VBO* vbo = nullptr;
    EBO* ebo = nullptr;
    Shader* shader = nullptr;

    std::unordered_map<std::string, GLTexture*> textureCache;
    GLTexture* GetOrLoadTexture(const std::string& path);

    void DrawQuad(Rect rect, Color color, GLTexture* tex);
};

#endif
