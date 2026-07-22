#include "Backend.hpp"
#include "Renderer.hpp"

int main()
{
    Backend backend;

    if (!backend.init()) return 1;

    backend.run();
    backend.die();

    return 0;
}
