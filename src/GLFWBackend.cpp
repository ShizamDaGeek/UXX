#include "GLFWBackend.hpp"

// |=====================================================
// |---[Helper Functions]--------------------------------
// |=====================================================
void GLFWBackend::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    GLFWBackend* glfwBackend = static_cast<GLFWBackend*>(glfwGetWindowUserPointer(window));
    if (glfwBackend)
    {
        // Keep the renderer's screen-space constants in sync with the actual window/framebuffer sizes
        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        UXX::SCREEN_WIDTH  = (float)windowWidth;
        UXX::SCREEN_HEIGHT = (float)windowHeight;
        UXX::SCREEN_SCALE_X = (float)width  / (float)windowWidth;
        UXX::SCREEN_SCALE_Y = (float)height / (float)windowHeight;
    }
}
void GLFWBackend::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    GLFWBackend* glfwBackend = static_cast<GLFWBackend*>(glfwGetWindowUserPointer(window));
    if (!glfwBackend) return;

    // Track both the held state and a one-frame press/release flag per mouse button
    if (button >= 0 && button < 3)
    {
        if (action == GLFW_PRESS)
        {
            glfwBackend->mouseButtonDown[button] = true;
            glfwBackend->mouseButtonPressed[button] = true; // one-frame "just clicked" flag
        }
        else if (action == GLFW_RELEASE)
        {
            glfwBackend->mouseButtonDown[button] = false;
            glfwBackend->mouseButtonReleased[button] = true; // one-frame "just released" flag
        }
    }
}
void GLFWBackend::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    GLFWBackend* glfwBackend = static_cast<GLFWBackend*>(glfwGetWindowUserPointer(window));
    if (glfwBackend)
    {
        glfwBackend->mouseX = xpos;
        glfwBackend->mouseY = ypos;
    }
}

void GLFWBackend::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    GLFWBackend* glfwBackend = static_cast<GLFWBackend*>(glfwGetWindowUserPointer(window));
    if (glfwBackend)
    {
        glfwBackend->scrollX = xoffset;
        glfwBackend->scrollY = yoffset;
    }
}
void GLFWBackend::GetMouseInput()
{
    glfwGetCursorPos(window, &mouseX, &mouseY);
}

// |=====================================================
// |---[Constructer/Destructer]--------------------------
// |=====================================================
GLFWBackend::GLFWBackend() {}
GLFWBackend::~GLFWBackend() {}

// |=====================================================
// |---[Initlize]----------------------------------------
// |=====================================================
bool GLFWBackend::init()
{
    if (!glfwInit())
    {
        std::cerr << "Error trying to initialize GLFW \n";
        return false;
    }

    // ===[Request an OpenGL 4.6, core-profile, forward compatablity, context with debug output]===
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

    // ===[Hook up input callbacks and make this context current]===
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

    // ===[Record initial window size for the renderer's screen-space math]===
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    UXX::SCREEN_WIDTH  = (float)windowWidth;
    UXX::SCREEN_HEIGHT = (float)windowHeight;

    // ===[Framebuffer size can differ from window size on high-DPI displays, so track the scale]===
    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, frameBufferWidth, frameBufferHeight);
    UXX::SCREEN_SCALE_X = (float)frameBufferWidth  / (float)windowWidth;
    UXX::SCREEN_SCALE_Y = (float)frameBufferHeight / (float)windowHeight;

    UXX::SetWindowBackend(WindowBackend::GLFW);
    UXX::init();

    return true;
}

// |=====================================================
// |---[Run and Hide]------------------------------------
// |=====================================================
void GLFWBackend::run()
{
    // Loop until the user closes the window
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

        // Mouse States
        UXX::SetMouseState(mouseX, mouseY,
            mouseButtonDown[GLFW_MOUSE_BUTTON_LEFT],
            mouseButtonPressed[GLFW_MOUSE_BUTTON_LEFT],
            mouseButtonReleased[GLFW_MOUSE_BUTTON_LEFT],
            mouseButtonDown[GLFW_MOUSE_BUTTON_RIGHT],
            mouseButtonPressed[GLFW_MOUSE_BUTTON_RIGHT],
            mouseButtonReleased[GLFW_MOUSE_BUTTON_RIGHT],
            mouseButtonDown[GLFW_MOUSE_BUTTON_MIDDLE],
            mouseButtonPressed[GLFW_MOUSE_BUTTON_MIDDLE],
            mouseButtonReleased[GLFW_MOUSE_BUTTON_MIDDLE],
            scrollX, scrollY);

        // Specify the color of the background
    	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    	// Clean the back buffer and assign the new color to it
    	glClear(GL_COLOR_BUFFER_BIT);

    	// Draw UI
    	UXX::BeginPanel(Rect(0, 0, 1920, 1080, 0), Color(0.1f, 0.5f, 0.9f, 1.0f), "");

        Color normalColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
        Color hoveredColor = Color(0.5f, 0.5f, 0.5f, 1.0f);
        Color clickedColor = Color(0.0f, 0.0f, 0.0f, 1.0f);

        Color color1 = Color(0.3f, 0.9f, 0.5f, 1.0f);
        Color color2 = Color(0.5f, 0.3f, 0.9f, 1.0f);
        Color color3 = Color(0.9f, 0.5f, 0.3f, 1.0f);

        std::string scoutImagePath("../Images/scout.jpg");
        std::string catImagePath("../Images/cat.jpg");
        std::string fontPath("../Fonts/sandypixels_5x5_font2.ttf");

        static int intSliderValue = 75;
        static float floatSliderValue = 50.0f;
        static bool boolSwitchValue = false;

        if (UXX::Button(Rect(0, 0, 400, 400, 0), normalColor, hoveredColor, clickedColor, color1, 2.0f, "Cat", catImagePath, fontPath))
            std::cout << "Cat" << "\n";

        UXX::Image(Rect(500, 150, 250, 250, 0), Color(1.0f, 1.0f, 1.0f, 1.0f), scoutImagePath);

        UXX::IntSlider(Rect(0, 400, 80, 20, 0), color2, color1, intSliderValue, 1, 100, 1, color3, 1.3f, std::to_string(intSliderValue), "", fontPath);
        UXX::FloatSlider(Rect(0, 425, 80, 20, 0), color2, color1, floatSliderValue, 1.0f, 100.0f, color3, 1.3f, std::to_string((int)floatSliderValue), "", fontPath);
        UXX::Switch(Rect(0, 450, 80, 20, 0), color1, color2, color3, boolSwitchValue, 1.0f, "On", "Off", "", fontPath);

        UXX::Text(Rect(600, 100, 80, 60, -45), color1, 2.5f, "Think FAST Chuckle Nuts!", fontPath);

    	UXX::EndPanel();

    	// Swap the back buffer with the front buffer
    	glfwSwapBuffers(window);
    }
}

// |=====================================================
// |---[Heavy: You are Dead]-----------------------------
// |=====================================================
void GLFWBackend::die()
{
    UXX::BlowUp();

    glfwDestroyWindow(window);
    glfwTerminate();
}
