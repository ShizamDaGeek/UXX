#include "UXX/Backend.hpp"

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

        window = glfwCreateWindow(1920, 1080, "UXX Testing Window", NULL, NULL);
        if (!window)
        {
                glfwTerminate();
                std::cerr << "Error trying to create window \n";
        }
        glfwMakeContextCurrent(window);


        // GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        // glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        // glCompileShader(vertexShader);

        // GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        // glShaderSource(fragmentShader, 1, &fragmentShader, NULL);
        // glCompileShader(fragmentShader);

        // GLuint shaderProgram = glCreateProgram();
        // glAttachShader(shaderProgram, vertexShader);
        // glAttachShader(shaderProgram, fragmentShader);
        // glLinkProgram(shaderProgram);

        // glDeleteShader(vertexShader);
        // glDeleteShader(fragmentShader);


        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
                std::cerr << "Error trying to initialize GLAD\n";
                return false;
        }

        return true;
}
void Backend::run()
{
        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
                /* Render here */
                glClear(GL_COLOR_BUFFER_BIT);
                /* Swap front and back buffers */
                glfwSwapBuffers(window);
                /* Poll for and process events */
                glfwPollEvents();
        }
}
void Backend::blowup()
{
        glfwTerminate();
}
