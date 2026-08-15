#include "UXXBackendGLFW.hpp"
#include "UXX.hpp"

int main()
{
    UXXBackendGLFW uxxBackendGLFW;
    if (!uxxBackendGLFW.init()) return 1;

    uxxBackendGLFW.run([&]()
    {
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
        std::string electionsImagePath("../UXXAssets/Images/elections.jpg");
        std::string hamImagePath("../UXXAssets/Images/hamster.png");
        std::string ballImagePath("../UXXAssets/Images/ball.png");
        std::string fontPath("../UXXAssets/Fonts/sandypixels_5x5_font2.ttf");

        static int intSliderValue = 75;
        static float floatSliderValue = 50.0f;
        static bool boolSwitchValue = false;

        if (UXX::Button(Rect(0, 0, 400, 400, 0), normalColor, hoveredColor, clickedColor, color1, 2.0f, "Cat", catImagePath, fontPath))
            std::cout << "Cat" << "\n";

        UXX::Image(Rect(500, 150, 250, 250, 0), Color(1.0f, 1.0f, 1.0f, 1.0f), scoutImagePath);
        UXX::Image(Rect(800, 550, 300, 300, 0), Color(1.0f, 1.0f, 1.0f, 1.0f), supermanImagePath);

        UXX::IntSlider(Rect(0, 400, 400, 200, 0), intSliderValue, 1, 100, 1, color2, color1, color3, 2, std::to_string(intSliderValue), electionsImagePath, hamImagePath, fontPath);
        UXX::FloatSlider(Rect(0, 600, 400, 200, 0), floatSliderValue, 1.0f, 100.0f, 1.0f, color2, color1, color3, 2, std::to_string((int)floatSliderValue), electionsImagePath, hamImagePath, fontPath);
        UXX::Switch(Rect(0, 800, 400, 200, 0), boolSwitchValue, color1, color2, color3, 2, "On", "Off", catImagePath, scoutImagePath, fontPath);

        UXX::Text(Rect(600, 100, 80, 60, -45), color1, 2.5f, "Think FAST Chuckle Nuts!", fontPath);

        UXX::EndPanel();
    });

    uxxBackendGLFW.die();

    return 0;
}
