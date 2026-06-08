#include "Backend.hpp"

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
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Error trying to initialize GLAD\n";
        return false;
    }
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    renderer = new Renderer();
    renderer->init();
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

    	// Draw the UXX
    	renderer->drawUXXPanel(Rect(0, 0, 80, 60, 0), Color(0.1f, 0.5f, 0.9f, 1.0f));

    	// Swap the back buffer with the front buffer
    	glfwSwapBuffers(window);
    	// Take care of all GLFW events
    	glfwPollEvents();
    }
}
void Backend::blowup()
{
    renderer->blowup();
    delete renderer;

    glfwDestroyWindow(window);
    glfwTerminate();
}
