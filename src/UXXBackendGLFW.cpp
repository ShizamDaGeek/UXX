#include "UXXBackendGLFW.hpp"

// |=====================================================
// |---[Helper Functions]--------------------------------
// |=====================================================
void UXXBackendGLFW::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    UXXBackendGLFW* uxxBackendGLFW = static_cast<UXXBackendGLFW*>(glfwGetWindowUserPointer(window));
    if (uxxBackendGLFW)
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
void UXXBackendGLFW::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    UXXBackendGLFW* uxxBackendGLFWInstance = static_cast<UXXBackendGLFW*>(glfwGetWindowUserPointer(window));
    if (uxxBackendGLFWInstance)
    {
        uxxBackendGLFWInstance->mouseState.cursorPositionX = xpos;
        uxxBackendGLFWInstance->mouseState.cursorPositionY = ypos;
    }
}

void UXXBackendGLFW::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    UXXBackendGLFW* uxxBackendGLFWInstance = static_cast<UXXBackendGLFW*>(glfwGetWindowUserPointer(window));
    if (uxxBackendGLFWInstance)
    {
        uxxBackendGLFWInstance->mouseState.scrollWheelDeltaX = xoffset;
        uxxBackendGLFWInstance->mouseState.scrollWheelDeltaY = yoffset;
    }
}
void UXXBackendGLFW::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    UXXBackendGLFW* uxxBackendGLFWInstance = static_cast<UXXBackendGLFW*>(glfwGetWindowUserPointer(window));
    if (!uxxBackendGLFWInstance) return;
    if (button < 0 || button >= TOTAL_SUPPORTED_MOUSE_BUTTON_COUNT) return;

    // Fires immediately on the real OS event, independent of render frame pacing.
    if (action == GLFW_PRESS)
        uxxBackendGLFWInstance->mouseState.buttonPressedSinceLastPoll[button] = true;
    else if (action == GLFW_RELEASE)
        uxxBackendGLFWInstance->mouseState.buttonReleasedSinceLastPoll[button] = true;
}
void UXXBackendGLFW::PollMouseState()
{
    for (int mouseButtonIndex = 0; mouseButtonIndex < TOTAL_SUPPORTED_MOUSE_BUTTON_COUNT; mouseButtonIndex++)
    {
        bool buttonIsCurrentlyDown = glfwGetMouseButton(window, mouseButtonIndex) == GLFW_PRESS;

        // Press/release edges come from the callback buffer, not from comparing
        // this poll's instantaneous state to last poll's - that comparison is
        // exactly what dropped clicks that landed between two frames.
        mouseState.buttonPressedThisFrame[mouseButtonIndex]  = mouseState.buttonPressedSinceLastPoll[mouseButtonIndex];
        mouseState.buttonReleasedThisFrame[mouseButtonIndex] = mouseState.buttonReleasedSinceLastPoll[mouseButtonIndex];
        mouseState.buttonHeldThisFrame[mouseButtonIndex]     = buttonIsCurrentlyDown;

        // Consumed for this frame - clear until the next real GLFW event
        mouseState.buttonPressedSinceLastPoll[mouseButtonIndex]  = false;
        mouseState.buttonReleasedSinceLastPoll[mouseButtonIndex] = false;
    }
}

// |=====================================================
// |---[Constructer/Destructer]--------------------------
// |=====================================================
UXXBackendGLFW::UXXBackendGLFW() {}
UXXBackendGLFW::~UXXBackendGLFW() {}

// |=====================================================
// |---[Initlize]----------------------------------------
// |=====================================================
bool UXXBackendGLFW::init()
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
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
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
void UXXBackendGLFW::run(std::function<void()> drawUserInterfaceCallback)
{
    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        mouseState.scrollWheelDeltaX = 0.0;
        mouseState.scrollWheelDeltaY = 0.0;

        glfwPollEvents();
        PollMouseState();

        // ===[Mouse States]===
        UXX::SetMouseState(mouseState.cursorPositionX, mouseState.cursorPositionY,
            mouseState.buttonHeldThisFrame[GLFW_MOUSE_BUTTON_LEFT],
            mouseState.buttonPressedThisFrame[GLFW_MOUSE_BUTTON_LEFT],
            mouseState.buttonReleasedThisFrame[GLFW_MOUSE_BUTTON_LEFT],
            mouseState.buttonHeldThisFrame[GLFW_MOUSE_BUTTON_RIGHT],
            mouseState.buttonPressedThisFrame[GLFW_MOUSE_BUTTON_RIGHT],
            mouseState.buttonReleasedThisFrame[GLFW_MOUSE_BUTTON_RIGHT],
            mouseState.buttonHeldThisFrame[GLFW_MOUSE_BUTTON_MIDDLE],
            mouseState.buttonPressedThisFrame[GLFW_MOUSE_BUTTON_MIDDLE],
            mouseState.buttonReleasedThisFrame[GLFW_MOUSE_BUTTON_MIDDLE],
            mouseState.scrollWheelDeltaX, mouseState.scrollWheelDeltaY);

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

        double mouseX, mouseY;
        UXX::GetMouse(&mouseX, &mouseY);

    	// ===[Draw UI]===
        if (drawUserInterfaceCallback)
            drawUserInterfaceCallback();

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
void UXXBackendGLFW::die()
{
    UXX::BlowUp();

    glfwDestroyWindow(window);
    glfwTerminate();
}
