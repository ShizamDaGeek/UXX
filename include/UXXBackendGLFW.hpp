#ifndef UXXBACKENDGLFW_HPP
#define UXXBACKENDGLFW_HPP

// The Backend class will take care of the window making and initialization/clearing libraries

#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <optional>

#include "UXX.hpp"
#include "internal/Shader.hpp"
#include "internal/VAO.hpp"
#include "internal/VBO.hpp"
#include "internal/EBO.hpp"

// GLFW_MOUSE_BUTTON_LAST is the highest valid button index GLFW defines
constexpr int TOTAL_SUPPORTED_MOUSE_BUTTON_COUNT = GLFW_MOUSE_BUTTON_LAST + 1;

struct MouseState
{
    double cursorPositionX = 0.0, cursorPositionY = 0.0;
    double scrollWheelDeltaX = 0.0, scrollWheelDeltaY = 0.0;

    // Indexed by GLFW_MOUSE_BUTTON_* (0 .. TOTAL_SUPPORTED_MOUSE_BUTTON_COUNT-1).
    // Every button GLFW knows about is tracked automatically, so nothing here
    // needs to be added by hand when a new button shows up on some mouse.
    std::array<bool, TOTAL_SUPPORTED_MOUSE_BUTTON_COUNT> buttonHeldThisFrame{};     // Held this frame
    std::array<bool, TOTAL_SUPPORTED_MOUSE_BUTTON_COUNT> buttonPressedThisFrame{};  // Just went down this frame
    std::array<bool, TOTAL_SUPPORTED_MOUSE_BUTTON_COUNT> buttonReleasedThisFrame{}; // Just went up this frame

    // Event-driven, filled by MouseButtonCallback the instant GLFW reports a
    // press/release, so a click that happens between two frames still gets caught
    // instead of being missed by glfwGetMouseButton's instantaneous poll.
    std::array<bool, TOTAL_SUPPORTED_MOUSE_BUTTON_COUNT> buttonPressedSinceLastPoll{};
    std::array<bool, TOTAL_SUPPORTED_MOUSE_BUTTON_COUNT> buttonReleasedSinceLastPoll{};

    // Convenience accessors so callers can just ask about a button index
    // without reaching into the arrays directly.
    bool IsButtonHeld(int glfwMouseButtonIndex) const { return buttonHeldThisFrame[glfwMouseButtonIndex]; }
    bool WasButtonPressed(int glfwMouseButtonIndex) const { return buttonPressedThisFrame[glfwMouseButtonIndex]; }
    bool WasButtonReleased(int glfwMouseButtonIndex) const { return buttonReleasedThisFrame[glfwMouseButtonIndex]; }
};

class UXXBackendGLFW
{
public:
    UXXBackendGLFW();
    ~UXXBackendGLFW();

    bool init();
    void run(std::function<void()> drawUserInterfaceCallback);
    void die();

    // Single source of truth for mouse input this frame.
    const MouseState& GetMouseState() const { return mouseState; }

    // Bypasses GLFW polling entirely so unit tests can drive widgets without a real window/device.
    void InjectMouseState(const MouseState& state) { mouseState = state; }

    unsigned int SCREEN_WIDTH = 1920;
    unsigned int SCREEN_HEIGHT = 1080;

private:
    GLFWwindow* window;
    MouseState mouseState;

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    // Walks every GLFW mouse button index automatically each frame
    void PollMouseState();
};

#endif
