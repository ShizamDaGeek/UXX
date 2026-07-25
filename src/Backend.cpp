#include "Backend.hpp"
#include <GLFW/glfw3.h>

// |=====================================================
// |---[Helper Functions]--------------------------------
// |=====================================================
void Backend::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    Backend* backend = static_cast<Backend*>(glfwGetWindowUserPointer(window));
    if (backend)
    {
        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        UXX::SCREEN_WIDTH  = (float)windowWidth;
        UXX::SCREEN_HEIGHT = (float)windowHeight;
        UXX::SCREEN_SCALE_X = (float)width  / (float)windowWidth;
        UXX::SCREEN_SCALE_Y = (float)height / (float)windowHeight;
    }
}
void Backend::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Backend* backend = static_cast<Backend*>(glfwGetWindowUserPointer(window));
    if (!backend) return;

    if (button >= 0 && button < 3)
    {
        if (action == GLFW_PRESS)
        {
            backend->mouseButtonDown[button] = true;
            backend->mouseButtonPressed[button] = true; // one-frame "just clicked" flag
        }
        else if (action == GLFW_RELEASE)
        {
            backend->mouseButtonDown[button] = false;
            backend->mouseButtonReleased[button] = true; // one-frame "just released" flag
        }
    }
}
void Backend::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    Backend* backend = static_cast<Backend*>(glfwGetWindowUserPointer(window));
    if (backend)
    {
        backend->mouseX = xpos;
        backend->mouseY = ypos;
    }
}

void Backend::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Backend* backend = static_cast<Backend*>(glfwGetWindowUserPointer(window));
    if (backend)
    {
        backend->scrollX = xoffset;
        backend->scrollY = yoffset;
    }
}
void Backend::GetMouseInput()
{
    glfwGetCursorPos(window, &mouseX, &mouseY);
}

// |=====================================================
// |---[Constructer/Destructer]--------------------------
// |=====================================================
Backend::Backend() {}
Backend::~Backend() {}

// |=====================================================
// |---[Initlize]----------------------------------------
// |=====================================================
bool Backend::init()
{
    if (!glfwInit())
    {
        std::cerr << "Error trying to initialize GLFW \n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "UXX Testing Window", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        std::cerr << "Error trying to create window \n";
        return false;
    }
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Error trying to initialize GLAD\n";
        return false;
    }

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    UXX::SCREEN_WIDTH  = (float)windowWidth;
    UXX::SCREEN_HEIGHT = (float)windowHeight;

    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, frameBufferWidth, frameBufferHeight);
    UXX::SCREEN_SCALE_X = (float)frameBufferWidth  / (float)windowWidth;
    UXX::SCREEN_SCALE_Y = (float)frameBufferHeight / (float)windowHeight;

    UXX::init();

    return true;
}

// |=====================================================
// |---[Run and Hide]------------------------------------
// |=====================================================
void Backend::run()
{
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        // Reset one-frame flags before polling new events
        for (int i = 0; i < 3; i++)
        {
            mouseButtonPressed[i] = false;
            mouseButtonReleased[i] = false;
        }
        scrollX = 0.0;
        scrollY = 0.0;

        glfwPollEvents();

        UXX::SetMouseState(mouseX, mouseY,
            mouseButtonPressed[GLFW_MOUSE_BUTTON_LEFT],
            mouseButtonPressed[GLFW_MOUSE_BUTTON_RIGHT],
            mouseButtonPressed[GLFW_MOUSE_BUTTON_MIDDLE]);

        // Specify the color of the background
    	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    	// Clean the back buffer and assign the new color to it
    	glClear(GL_COLOR_BUFFER_BIT);

    	// Draw shit
    	UXX::BeginPanel(Rect(0, 0, 800, 600, 0), Color(0.1f, 0.5f, 0.9f, 1.0f));

        Color normalColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
        Color hoveredColor = Color(0.5f, 0.5f, 0.5f, 1.0f);
        Color clickedColor = Color(0.0f, 0.0f, 0.0f, 1.0f);

        static int intSliderValue = 1;
        static float floatSliderValue = 1.0f;
        static bool boolSwitchValue = false;

        if (UXX::DrawButton(Rect(0, 0, 100, 100, 0), normalColor, hoveredColor, clickedColor, "../Image/scout.jpg"))
        {
            std::cout << "Button Clicked" << "\n";
        }

        UXX::DrawImage(Rect(0, 50, 80, 60, 0), Color(1.0f, 1.0f, 1.0f, 0.5f), "../Image/scout.jpg");

        UXX::DrawIntSlider(Rect(50, 150, 80, 20, 0), Color(0.5f, 0.3f, 0.9f, 1.0f), Color(0.3f, 0.9f, 0.5f, 1.0f), intSliderValue, 1, 100, 1);
        UXX::DrawFloatSlider(Rect(50, 175, 80, 20, 0), Color(0.5f, 0.3f, 0.9f, 1.0f), Color(0.3f, 0.9f, 0.5f, 1.0f), floatSliderValue, 1.0f, 100.0f);
        UXX::DrawSwitch(Rect(50, 200, 80, 20, 0), Color(0.5f, 0.3f, 0.9f, 1.0f), Color(0.3f, 0.9f, 0.5f, 1.0f), boolSwitchValue);

    	UXX::EndPanel();

    	// Swap the back buffer with the front buffer
    	glfwSwapBuffers(window);
    }
}

// |=====================================================
// |---[Heavy: You are Dead]-----------------------------
// |=====================================================
void Backend::die()
{
    UXX::BlowUp();

    glfwDestroyWindow(window);
    glfwTerminate();
}
