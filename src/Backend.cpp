#include "Backend.hpp"

void Backend::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    Backend* backend = static_cast<Backend*>(glfwGetWindowUserPointer(window));
    if (backend && backend->renderer)
    {
        backend->renderer->SCREEN_WIDTH  = (float)width;
        backend->renderer->SCREEN_HEIGHT = (float)height;
    }
}

Backend::Backend() {}
Backend::~Backend() {}

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
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Error trying to initialize GLAD\n";
        return false;
    }
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    renderer = new Renderer();
    renderer->init();
    renderer->SCREEN_WIDTH  = (float)fbWidth;
    renderer->SCREEN_HEIGHT = (float)fbHeight;
    return true;
}
void Backend::run()
{
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        // Specify the color of the background
    	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    	// Clean the back buffer and assign the new color to it
    	glClear(GL_COLOR_BUFFER_BIT);

    	// Draw shit
    	renderer->BeginUXXPanel(Rect(0, 0, 800, 600, 0), Color(0.1f, 0.5f, 0.9f, 1.0f));
    	renderer->DrawUXXButton(Rect(0, 0, 80, 60, 0), Color(0.9f, 0.3f, 0.6f, 1.0f), "../Image/scout.jpg");
        renderer->DrawUXXImage(Rect(0, 0, 800, 600, 0), Color(0.9f, 0.3f, 0.6f, 1.0f), "../Image/scout.jpg");
    	renderer->EndUXXPanel();

    	// Swap the back buffer with the front buffer
    	glfwSwapBuffers(window);
    	// Take care of all GLFW events
    	glfwPollEvents();
    }
}
void Backend::blowup()
{
    renderer->BlowUp();
    delete renderer;

    glfwDestroyWindow(window);
    glfwTerminate();
}
