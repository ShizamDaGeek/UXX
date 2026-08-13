#ifndef UXX_HPP
#define UXX_HPP

// The Renderer class will handle pretty much all UI drawing related stuff

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <iostream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <vector>

#include "internal/Texture.hpp"
#include "internal/VAO.hpp"
#include "internal/VBO.hpp"
#include "internal/EBO.hpp"
#include "internal/Shader.hpp"
#include "internal/Font.hpp"

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

enum class SeparatorStyle { Solid, Dashed, Dotted };

enum class WindowBackend
{
    UNKNOWN,
    GLFW,
    SDL2,
    SDL3,
    SFML
};


namespace UXX
{
    inline float SCREEN_WIDTH = 1920.0f;
    inline float SCREEN_HEIGHT = 1080.0f;
    inline float SCREEN_SCALE_X = 1.0f;
    inline float SCREEN_SCALE_Y = 1.0f;

    inline Shader* textShader = nullptr;
    inline unsigned int textVAO, textVBO;

    struct GraphicsInfo
    {
        WindowBackend backend = WindowBackend::UNKNOWN;
        int glMajor = 0;
        int glMinor = 0;
        std::string glVersionString;
        std::string glVendor;
        std::string glRenderer;
    };

    inline GraphicsInfo graphicsInfo;
    const GraphicsInfo& GetGraphicsInfo();

    void SetWindowBackend(WindowBackend backend);

    void init();
    void InitTextRendering();

    void SetMouseState(double mouseX, double mouseY,
        bool leftDown, bool leftPressed, bool leftReleased,
        bool rightDown, bool rightPressed, bool rightReleased,
        bool middleDown, bool middlePressed, bool middleReleased,
        double scrollDeltaX, double scrollDeltaY);
    void GetScrollDelta(double& outX, double& outY);
    void SetKeyState(bool leftArrowPressed, bool rightArrowPressed, bool upArrowPressed, bool downArrowPressed);
    bool MouseHoveredOverWidget();

    // This is the order that the structs sould go:
    // Rect -> Color -> Anything realted to the UI itself _
    //                                                     |
    //  --- Font path <- Image path <- Text <- Text Size <-
    // |
    // |                             /TT\  /
    // |                         O*=[_||_]<--
    //  -------_-_---____----- ..oO  U  U  \

    void BeginPanel(Rect PanelRect, Color PanelColor, std::string PanelImagePath);
    void EndPanel();

    bool Button(Rect ButtonRect, Color ButtonColor, Color ButtonHoverColor, Color ButtonClickedColor, Color ButtonTextColor, float ButtonTextSize, std::string ButtonTextItself, std::string ButtonImagePath, std::string ButtonFontPath);
    bool IntSlider(Rect IntSliderRect, Color IntTrackColor, Color IntHandleColor, int& value, int minIntValue, int maxIntValue, int intStep, Color IntSliderTextColor, float IntSliderTextSize, std::string IntSliderTextItself, std::string IntSliderImagePath, std::string IntSliderFontPath);
    bool FloatSlider(Rect FloatSliderRect, Color FloatTrackColor, Color FloatHandleColor, float& value, float minFloatValue, float maxFloatValue, Color FloatSliderTextColor, float FloatSliderTextSize, std::string FloatSliderTextItself, std::string FloatSliderImagePath, std::string FloatSliderFontPath);
    bool Switch(Rect SwitchRect, Color SwitchOnColor, Color SwitchOffColor, Color SwitchTextColor, bool& value, float SwitchTextSize, std::string SwitchOnTextItself, std::string SwitchOffTextItself, std::string SwitchImagePath, std::string SwitchFontPath);

    void Image(Rect ImageRect, Color ImageColor, std::string ImagePath);
    void Separator(Rect SeparatorRect, Color SeparatorColor);
    void Text(Rect TextRect, Color TextColor, float TextSize, std::string TextItself, std::string FontPath);

    void BlowUp();
};

#endif
