#include "UXXBackendGLFW.hpp"
#include "UXX.hpp"

int main()
{
    UXXBackendGLFW uxxBackendGLFW;
    if (!uxxBackendGLFW.init()) return 1;

    uxxBackendGLFW.run([&]()
    {
        // ===[Wacky UI]===
        UXX::BeginPanel(Rect(0, 0, 1920, 1080, 0), Color(0.1f, 0.5f, 0.9f, 1.0f), "");

        std::string scoutImagePath("../UXXAssets/Images/scout.jpg");
        std::string catImagePath("../UXXAssets/Images/cat.jpg");
        std::string supermanImagePath("../UXXAssets/Images/transparent_image.png");
        std::string electionsImagePath("../UXXAssets/Images/elections.jpg");
        std::string hamImagePath("../UXXAssets/Images/hamster.png");
        std::string fontPath("../UXXAssets/Fonts/sandypixels_5x5_font2.ttf");

        static int intSliderValue = 75;
        static float floatSliderValue = 50.0f;
        static bool boolSwitchValue = false;

        // ===[Styles]===
        UXX::ButtonStyle catButtonStyle
        {
            .normalColor  = Color(1.0f, 1.0f, 1.0f, 1.0f),
            .hoveredColor = Color(0.5f, 0.5f, 0.5f, 1.0f),
            .clickedColor = Color(0.0f, 0.0f, 0.0f, 1.0f),
            .textColor    = Color(0.3f, 0.9f, 0.5f, 1.0f),
            .textSize     = 2.0f,
            .fontPath     = fontPath,
            .imagePath    = catImagePath
        };

        UXX::SliderStyle imageSliderStyle
        {
            .trackColor      = Color(0.5f, 0.3f, 0.9f, 1.0f),
            .handleColor     = Color(0.3f, 0.9f, 0.5f, 1.0f),
            .textColor       = Color(0.9f, 0.5f, 0.3f, 1.0f),
            .textSize        = 2.0f,
            .fontPath        = fontPath,
            .trackImagePath  = electionsImagePath,
            .handleImagePath = hamImagePath
        };

        UXX::SwitchStyle catSwitchStyle
        {
            .onColor      = Color(0.3f, 0.9f, 0.5f, 1.0f),
            .offColor     = Color(0.5f, 0.3f, 0.9f, 1.0f),
            .textColor    = Color(0.9f, 0.5f, 0.3f, 1.0f),
            .textSize     = 2.0f,
            .fontPath     = fontPath,
            .onImagePath  = catImagePath,
            .offImagePath = scoutImagePath
        };

        UXX::TextStyle titleTextStyle
        {
            .color    = Color(0.3f, 0.9f, 0.5f, 1.0f),
            .size     = 2.5f,
            .fontPath = fontPath
        };

        UXX::ImageStyle scoutImageStyle    { .imagePath = scoutImagePath };
        UXX::ImageStyle supermanImageStyle { .imagePath = supermanImagePath };

        // ===[Widgets]===
        if (UXX::Button(Rect(0, 0, 400, 400, 0), catButtonStyle, "Cat"))
            std::cout << "Cat" << "\n";

        UXX::Image(Rect(500, 150, 250, 250, 0), scoutImageStyle);
        UXX::Image(Rect(800, 550, 300, 300, 0), supermanImageStyle);

        UXX::IntSlider(Rect(0, 400, 400, 200, 0), intSliderValue, 1, 100, 1, imageSliderStyle, std::to_string(intSliderValue));
        UXX::FloatSlider(Rect(0, 600, 400, 200, 0), floatSliderValue, 1.0f, 100.0f, 1.0f, imageSliderStyle, std::to_string((int)floatSliderValue));
        UXX::Switch(Rect(0, 800, 400, 200, 0), boolSwitchValue, catSwitchStyle, "On", "Off");

        UXX::Text(Rect(600, 100, 80, 60, -45), titleTextStyle, "Think FAST Chuckle Nuts!");

        UXX::EndPanel();

        // ===[Serious UI]===
    });

    uxxBackendGLFW.die();

    return 0;
}
