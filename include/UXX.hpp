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

    Rect(float xPos, float yPos, float width = 800, float height = 600, float rotation = 0)
        : xPos(xPos), yPos(yPos), width(width), height(height), rotation(rotation) {}
};
struct Color
{
    float red, green, blue, alpha;

    Color(float red = 1, float green = 1, float blue = 1, float alpha = 1)
        : red(red), green(green), blue(blue), alpha(alpha) {}
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
enum class TextAlign { Left, Center, Right };
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

    // ===[Sensible defaults so callers don't have to specify every param]===
    inline Color DefaultColor        { 0.20f, 0.20f, 0.20f, 1.0f };
    inline Color DefaultHoverColor   { 0.30f, 0.30f, 0.30f, 1.0f };
    inline Color DefaultClickedColor { 0.15f, 0.15f, 0.15f, 1.0f };
    inline Color DefaultTextColor    { 1.0f, 1.0f, 1.0f, 1.0f };
    inline std::string DefaultFontPath = "../UXXAssets/Fonts/sandypixels_5x5_font2.ttf";

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
    void GetMouse(double* x = nullptr, double* y = nullptr,
        bool* leftDown = nullptr, bool* leftPressed = nullptr, bool* leftReleased = nullptr,
        bool* rightDown = nullptr, bool* rightPressed = nullptr, bool* rightReleased = nullptr,
        bool* middleDown = nullptr, bool* middlePressed = nullptr, bool* middleReleased = nullptr,
        double* scrollX = nullptr, double* scrollY = nullptr);
    void GetScrollDelta(double& outX, double& outY);
    void SetKeyState(bool leftArrowPressed, bool rightArrowPressed, bool upArrowPressed, bool downArrowPressed);
    bool MouseHoveredOverWidget();

    //                               /TT\  /
    //                           O*=[_||_]<--
    // *-------_-_---____----- ..oO  U  U  \

    void BeginPanel(Rect PanelRect, Color PanelColor, std::string PanelImagePath);
    void EndPanel();

    bool Button(Rect ButtonRect,
                Color ButtonColor = DefaultColor,
                Color ButtonHoverColor = DefaultHoverColor,
                Color ButtonClickedColor = DefaultClickedColor,
                Color ButtonTextColor = DefaultTextColor,
                float ButtonTextSize = 2,
                std::string ButtonTextItself = "",
                std::string ButtonImagePath = "",
                std::string ButtonFontPath = DefaultFontPath);
    bool IntSlider(Rect IntSliderRect,
                    int& value,
                    int minIntValue,
                    int maxIntValue,
                    int intStep = 1,
                    Color IntTrackColor = DefaultColor,
                    Color IntHandleColor = DefaultHoverColor,
                    Color IntSliderTextColor = DefaultTextColor,
                    float IntSliderTextSize = 2,
                    std::string IntSliderTextItself = "",
                    std::string IntSliderTrackImagePath = "",
                    std::string IntSliderHandleImagePath = "",
                    std::string IntSliderFontPath = DefaultFontPath);
    bool FloatSlider(Rect FloatSliderRect,
                    float& value,
                    float minFloatValue,
                    float maxFloatValue,
                    float floatStep = 1.0f,
                    Color FloatTrackColor = DefaultColor,
                    Color FloatHandleColor = DefaultHoverColor,
                    Color FloatSliderTextColor = DefaultTextColor,
                    float FloatSliderTextSize = 2,
                    std::string FloatSliderTextItself = "",
                    std::string FloatSliderTrackImagePath = "",
                    std::string FloatSliderHandleImagePath = "",
                    std::string FloatSliderFontPath = DefaultFontPath);
    bool Switch(Rect SwitchRect,
                bool& value,
                Color SwitchOnColor = DefaultColor,
                Color SwitchOffColor = DefaultClickedColor,
                Color SwitchTextColor = DefaultTextColor,
                float SwitchTextSize = 2,
                std::string SwitchOnTextItself = "",
                std::string SwitchOffTextItself = "",
                std::string SwitchOnImagePath = "",
                std::string SwitchOffImagePath = "",
                std::string SwitchFontPath = DefaultFontPath);

    void Image(Rect ImageRect, Color ImageColor, std::string ImagePath);
    void Separator(Rect SeparatorRect, Color SeparatorColor);
    void Text(Rect TextRect, Color TextColor, float TextSize, std::string TextItself, std::string FontPath);

    void BlowUp();
};

#endif
