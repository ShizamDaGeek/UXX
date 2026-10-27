#include "UXXBackendGLFW.hpp"
#include "UXX.hpp"

/*
    *--------------------------------------------------------*
    |                      Hello World!                      |
    |     This is a C++ Libaray is made by ShizamDa_Geek     |
    .--------------------------------------------------------.
*/

int main()
{
    UXXBackendGLFW uxxBackendGLFW;
    if (!uxxBackendGLFW.init()) return 1;

    uxxBackendGLFW.run([&]()
    {
        // ===[Wacky UI]===
//         UXX::BeginPanel(Rect(0, 0, 1920, 1080, 0), Color(0.1f, 0.5f, 0.9f, 1.0f), "");
//
//         // ===[Variables]===
//         std::string scoutImagePath("../UXXAssets/Images/scout.jpg");
//         std::string catImagePath("../UXXAssets/Images/cat.jpg");
//         std::string supermanImagePath("../UXXAssets/Images/transparent_image.png");
//         std::string electionsImagePath("../UXXAssets/Images/elections.jpg");
//         std::string hamImagePath("../UXXAssets/Images/hamster.png");
//         std::string fontPath("../UXXAssets/Fonts/sandypixels_5x5_font2.ttf");
//
//         static int intSliderValue = 75;
//         static float floatSliderValue = 50.0f;
//         static bool boolSwitchValue = false;
//
//         // ===[Styles]===
//         UXX::ButtonStyle catButtonStyle
//         {
//             .normalColor  = Color(1.0f, 1.0f, 1.0f, 1.0f),
//             .hoveredColor = Color(0.5f, 0.5f, 0.5f, 1.0f),
//             .clickedColor = Color(0.0f, 0.0f, 0.0f, 1.0f),
//             .textColor    = Color(0.3f, 0.9f, 0.5f, 1.0f),
//             .textSize     = 2.0f,
//             .fontPath     = fontPath,
//             .imagePath    = catImagePath
//         };
//
//         UXX::SliderStyle imageSliderStyle
//         {
//             .trackColor      = Color(0.5f, 0.3f, 0.9f, 1.0f),
//             .handleColor     = Color(0.3f, 0.9f, 0.5f, 1.0f),
//             .textColor       = Color(0.9f, 0.5f, 0.3f, 1.0f),
//             .textSize        = 2.0f,
//             .fontPath        = fontPath,
//             .trackImagePath  = electionsImagePath,
//             .handleImagePath = hamImagePath
//         };
//
//         UXX::SwitchStyle catSwitchStyle
//         {
//             .onColor      = Color(0.3f, 0.9f, 0.5f, 1.0f),
//             .offColor     = Color(0.5f, 0.3f, 0.9f, 1.0f),
//             .textColor    = Color(0.9f, 0.5f, 0.3f, 1.0f),
//             .textSize     = 2.0f,
//             .fontPath     = fontPath,
//             .onImagePath  = catImagePath,
//             .offImagePath = scoutImagePath
//         };
//
//         UXX::TextStyle titleTextStyle
//         {
//             .color    = Color(0.3f, 0.9f, 0.5f, 1.0f),
//             .size     = 2.5f,
//             .fontPath = fontPath
//         };
//
//         UXX::ImageStyle scoutImageStyle    { .imagePath = scoutImagePath };
//         UXX::ImageStyle supermanImageStyle { .imagePath = supermanImagePath };
//
//         // ===[Widgets]===
//         if (UXX::Button(Rect(0, 0, 400, 400, 0), catButtonStyle, "Cat"))
//             std::cout << "Cat" << "\n";
//
//         UXX::Image(Rect(500, 150, 250, 250, 0), scoutImageStyle);
//         UXX::Image(Rect(800, 550, 300, 300, 0), supermanImageStyle);
//
//         UXX::IntSlider(Rect(0, 400, 400, 200, 0), intSliderValue, 1, 100, 1, imageSliderStyle, std::to_string(intSliderValue));
//         UXX::FloatSlider(Rect(0, 600, 400, 200, 0), floatSliderValue, 1.0f, 100.0f, 1.0f, imageSliderStyle, std::to_string((int)floatSliderValue));
//         UXX::Switch(Rect(0, 800, 400, 200, 0), boolSwitchValue, catSwitchStyle, "On", "Off");
//
//         UXX::Text(Rect(600, 100, 80, 60, -45), titleTextStyle, "Think FAST Chuckle Nuts!");
//
//         UXX::EndPanel();

        // ===[Serious UI]===
        UXX::BeginPanel(Rect(0, 0, 1920, 1080, 0), Color(0.1f, 0.5f, 0.9f, 1.0f), "");

        // ===[Variables]===
        static std::string multiLineTextValue = "";
        static float numberFieldValue = 5.0f;
        static std::string codeTextValue = "";

        static float inventoryScrollOffset = 0.0f;
        const float inventoryContentHeight = 1200.0f;
        Rect inventoryScrollBoxRect(100, 500, 400, 300);

        static int selectedWeaponIndex = -1;   // -1 means nothing selected yet, so the placeholder shows
        const std::vector<std::string> weaponLabelList =
        {
            "Sword", "Axe", "Bow", "Crossbow", "Spear", "Dagger", "Mace", "Staff"
        };

        // ===[Styles]===
        UXX::InputFieldStyle inputFieldStyle;
        UXX::CodeInputFieldStyle codeInputFieldStyle
        {
            .roundBracketColor  = Color(1.0f, 0.8f, 0.2f),
            .squareBracketColor = Color(0.4f, 0.8f, 1.0f),
            .curlyBracketColor  = Color(1.0f, 0.4f, 0.8f),
            .angleBracketColor  = Color(0.5f, 1.0f, 0.5f)
        };
        UXX::ButtonStyle buttonStyle
        {
            .normalColor  = Color(1.0f, 1.0f, 1.0f, 1.0f),
            .hoveredColor = Color(0.5f, 0.5f, 0.5f, 1.0f),
            .clickedColor = Color(0.0f, 0.0f, 0.0f, 1.0f),
        };
        UXX::ScrollBoxStyle scrollBoxStyle;
        UXX::DropDownStyle dropDownStyle;
        dropDownStyle.maxVisibleItems = 5;

        UXX::SeparatorStyle horizontalSeparatorStyle;
        horizontalSeparatorStyle.lineColor = Color(0.8f, 0.8f, 0.8f, 1.0f);
        horizontalSeparatorStyle.thickness = 2.0f;

        UXX::SeparatorStyle diagonalSeparatorStyle;
        diagonalSeparatorStyle.lineColor = Color(1.0f, 0.4f, 0.4f, 1.0f);
        diagonalSeparatorStyle.thickness = 4.0f;

        UXX::ToolTipStyle toolTipStyle;

        // ===[Widgets]===

        UXX::MultiInputField(Rect(50, 50, 400, 120), multiLineTextValue, inputFieldStyle);
        UXX::NumberInputField(Rect(50, 200, 200, 40), numberFieldValue, inputFieldStyle);
        UXX::CodeInputField(Rect(50, 260, 400, 150), codeTextValue, codeInputFieldStyle);

        UXX::BeginScrollBox(inventoryScrollBoxRect, inventoryScrollOffset, inventoryContentHeight, scrollBoxStyle);
        for (int itemIndex = 0; itemIndex < 20; itemIndex++)
        {
            // Subtracting the offset is what makes the content scroll
            float itemTopY = inventoryScrollBoxRect.yPos + itemIndex * 60.0f - inventoryScrollOffset;
            UXX::Button(Rect(inventoryScrollBoxRect.xPos + 10, itemTopY, 360, 50), buttonStyle, "Item " + std::to_string(itemIndex));
        }
        UXX::EndScrollBox();

        UXX::BeginDropDown(Rect(500, 50, 270, 40), selectedWeaponIndex, weaponLabelList, dropDownStyle, "Pick a weapon");
        UXX::EndDropDown();
        UXX::ToolTip(Rect(500, 50, 270, 40), toolTipStyle, "This is a Drop Down");

        // Rotation 0 gives a horizontal line across the whole panel at y = 200
        UXX::Separator(Rect(400, 200, 0, 0, 0.0f), horizontalSeparatorStyle);

        // Rotation 90 gives a vertical line across the whole panel at x = 400
        UXX::Separator(Rect(400, 300, 0, 0, 90.0f), horizontalSeparatorStyle);

        // Rotation 45 gives a diagonal line through the point (300, 250)
        UXX::Separator(Rect(300, 250, 0, 0, 45.0f), diagonalSeparatorStyle);

        UXX::EndPanel();
    });

    uxxBackendGLFW.die();

    return 0;
}
