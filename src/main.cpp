#include "UXX/Backend.hpp"
#include "UXX/Renderer.hpp"

int main()
{
        Backend backend;
        Renderer renderer;

        backend.init();

        backend.run();
        renderer.renderTriangle();

        backend.blowup();

        return 0;
}
