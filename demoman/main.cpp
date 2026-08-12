#include "GLFWBackend.hpp"
#include "UXX.hpp"

int main()
{
    GLFWBackend glfwBackend;

    if (!glfwBackend.init()) return 1;

    glfwBackend.run();
    glfwBackend.die();

    return 0;
}
