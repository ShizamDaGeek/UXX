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
    #ifndef NDEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    #else
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_FALSE);
    #endif

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
    glfwSwapInterval(1);
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
        // ===[Reset one-frame flags before polling new events]===
        for (int buttonIndex = 0; buttonIndex < 3; buttonIndex++)
        {
            mouseButtonPressed[buttonIndex] = false;
            mouseButtonReleased[buttonIndex] = false;
        }
        scrollX = 0.0;
        scrollY = 0.0;

        glfwPollEvents();

        // ===[Mouse States]===
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

        // ===[Arrow key polling]===
        static bool previousLeft = false, previousRight = false, previousUp = false, previousDown = false;
        static double leftHeldSince = 0.0, rightHeldSince = 0.0, upHeldSince = 0.0, downHeldSince = 0.0;
        static double leftLastRepeat = 0.0, rightLastRepeat = 0.0, upLastRepeat = 0.0, downLastRepeat = 0.0;

        const double repeatDelay = 0.4;   // seconds held before repeat kicks in
        const double repeatRate  = 0.05;  // seconds between repeats once repeating

        double now = glfwGetTime();

        bool currentLeft    = glfwGetKey(window, GLFW_KEY_LEFT)     == GLFW_PRESS;
        bool currentRight   = glfwGetKey(window, GLFW_KEY_RIGHT)    == GLFW_PRESS;
        bool currentUp      = glfwGetKey(window, GLFW_KEY_UP)       == GLFW_PRESS;
        bool currentDown    = glfwGetKey(window, GLFW_KEY_DOWN)     == GLFW_PRESS;

        auto ComputeFire = [&](bool current, bool& previous, double& heldSince, double& lastRepeat) -> bool
        {
            bool fire = false;
            if (current && !previous)
            {
                // Just pressed: fire immediately, start the hold timer
                fire = true;
                heldSince = now;
                lastRepeat = now;
            }
            else if (current && previous)
            {
                // Still held: fire again after the initial delay, then at the repeat rate
                if (now - heldSince >= repeatDelay && now - lastRepeat >= repeatRate)
                {
                    fire = true;
                    lastRepeat = now;
                }
            }
            previous = current;
            return fire;
        };

        bool fireLeft  = ComputeFire(currentLeft,  previousLeft,  leftHeldSince,  leftLastRepeat);
        bool fireRight = ComputeFire(currentRight, previousRight, rightHeldSince, rightLastRepeat);
        bool fireUp    = ComputeFire(currentUp,    previousUp,    upHeldSince,    upLastRepeat);
        bool fireDown  = ComputeFire(currentDown,  previousDown,  downHeldSince,  downLastRepeat);

        UXX::SetKeyState(fireLeft, fireRight, fireUp, fireDown);

        // Specify the color of the background
    	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    	// Clean the back buffer and assign the new color to it
    	glClear(GL_COLOR_BUFFER_BIT);

    	// ===[Draw UI]===
    	UXX::BeginPanel(Rect(0, 0, 1920, 1080, 0), Color(0.1f, 0.5f, 0.9f, 1.0f), "");

        Color normalColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
        Color hoveredColor = Color(0.5f, 0.5f, 0.5f, 1.0f);
        Color clickedColor = Color(0.0f, 0.0f, 0.0f, 1.0f);

        Color color1 = Color(0.3f, 0.9f, 0.5f, 1.0f);
        Color color2 = Color(0.5f, 0.3f, 0.9f, 1.0f);
        Color color3 = Color(0.9f, 0.5f, 0.3f, 1.0f);

        std::string scoutImagePath("../UXXAssets/Images/scout.jpg");
        std::string catImagePath("../UXXAssets/Images/cat.jpg");
        std::string supermanImagePath("../UXXAssets/Images/transparent_image.png");
        std::string fontPath("../UXXAssets/Fonts/sandypixels_5x5_font2.ttf");

        static int intSliderValue = 75;
        static float floatSliderValue = 50.0f;
        static bool boolSwitchValue = false;

        if (UXX::Button(Rect(0, 0, 400, 400, 0), normalColor, hoveredColor, clickedColor, color1, 2.0f, "Cat", catImagePath, fontPath))
            std::cout << "Cat" << "\n";

        UXX::Image(Rect(500, 150, 250, 250, 0), Color(1.0f, 1.0f, 1.0f, 1.0f), scoutImagePath);
        UXX::Image(Rect(800, 550, 300, 300, 0), Color(1.0f, 1.0f, 1.0f, 1.0f), supermanImagePath);

        UXX::IntSlider(Rect(0, 400, 140, 40, 0), intSliderValue, 1, 100, 1, color2, color1, color3, 2, std::to_string(intSliderValue), "", fontPath);
        UXX::FloatSlider(Rect(0, 450, 140, 40, 0), floatSliderValue, 1.0f, 100.0f, 1.0f, color2, color1, color3, 2, std::to_string((int)floatSliderValue), "", fontPath);
        UXX::Switch(Rect(0, 500, 140, 40, 0), boolSwitchValue, color1, color2, color3, 2, "On", "Off", catImagePath, scoutImagePath, fontPath);

        UXX::Text(Rect(600, 100, 80, 60, -45), color1, 2.5f, "Think FAST Chuckle Nuts!", fontPath);

    	UXX::EndPanel();

        // ===[Swap cursor when hovering an interactive widget]===
        static GLFWcursor* handCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        static bool cursorIsHand = false;
        bool wantHand = UXX::MouseHoveredOverWidget();
        if (wantHand != cursorIsHand)
        {
            glfwSetCursor(window, wantHand ? handCursor : nullptr);
            cursorIsHand = wantHand;
        }

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
