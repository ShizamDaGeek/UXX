#include "UXX.hpp"

namespace UXX
{
    // |=====================================================
    // |---[Private state]-----------------------------------
    // |=====================================================
    namespace
    {
        // ===[Shader stuff]===
        VAO* vao = nullptr;
        VBO* vbo = nullptr;
        EBO* ebo = nullptr;
        Shader* shader = nullptr;

        // ===[Caches]===
        std::unordered_map<std::string, GLTexture*> textureCache;
        GLTexture* GetOrLoadTexture(const std::string& path);

        std::unordered_map<std::string, Font*> fontCache;
        Font* GetOrLoadFont(const std::string& path, unsigned int pixelHeight = 48);

        // ===[Tile]===
        bool panelOpen = false;
        // ===[Keyboard]===
        bool leftArrowKeyPressed = false;
        bool rightArrowKeyPressed = false;
        bool upArrowKeyPressed = false;
        bool downArrowKeyPressed = false;

        // ===[Activate drag tracking]===
        const void* activeDragWidget = nullptr;

        // ===[Mouse]===
        bool leftMouseButtonDown = false;
        bool leftMouseButtonPressed = false;
        bool leftMouseButtonReleased = false;
        bool rightMouseButtonDown = false;
        bool rightMouseButtonPressed = false;
        bool rightMouseButtonReleased = false;
        bool middleMouseButtonDown = false;
        bool middleMouseButtonPressed = false;
        bool middleMouseButtonReleased = false;
        double mousePositionX, mousePositionY;

        // ===[Raw per-frame delta, for future scrollable widgets]===
        double scrollDeltaXState = 0.0;
        double scrollDeltaYState = 0.0;

        // ===[Square like Minecraft]===
        // Shared unit-quad geometry reused by every UI element
        float rectangleVertices[] =
        {
        //  positions               Color               TexCoords
            -0.5f, -0.5f, 0.0f,     1.0f, 1.0f, 1.0f,   0.0f, 0.0f, // Lower Left Corner
             0.5f, -0.5f, 0.0f,     1.0f, 1.0f, 1.0f,   1.0f, 0.0f, // Lower Right Corner
             0.5f,  0.5f, 0.0f,     1.0f, 1.0f, 1.0f,   1.0f, 1.0f, // Upper Right Corner
            -0.5f,  0.5f, 0.0f,     1.0f, 1.0f, 1.0f,   0.0f, 1.0f  // Upper Left Corner
        };

        unsigned int rectangleIndices[] =
        {
            0, 1, 2, // Upper Triangle
            2, 3, 0  // Lower Triangle
        };

        // |=====================================================
        // |---[Private Helper Functions]------------------------
        // |=====================================================
        // ===[tits]===
        GLTexture* GetOrLoadTexture(const std::string& path)
        {
            // Reuse an already-loaded texture if one exists for this path
            auto it = textureCache.find(path);
            if (it != textureCache.end()) return it->second;

            GLTexture* tex = new GLTexture(path.c_str(), "", 0);
            tex->texUnit(*shader, "tex0", 0);
            textureCache[path] = tex;

            return tex;
        }
        // ===[foot]===
        Font* GetOrLoadFont(const std::string& path, unsigned int pixelHeight)
        {
            // Key by path+size so the same font at different sizes stays distinct
            std::string key = path + "#" + std::to_string(pixelHeight);

            auto it = fontCache.find(key);
            if (it != fontCache.end()) return it->second;

            Font* font = new Font();
            if (!font->initFreeType(path.c_str(), pixelHeight))
            {
                std::cerr << "Failed to load font: " << path << "\n";
                delete font;
                fontCache[path] = nullptr;
                return nullptr;
            }

            fontCache[path] = font;
            return font;
        }
        // ===[Be there or be SQUARE]===
        void DrawQuad(Rect rect, Color color, GLTexture* texture)
        {
            shader->Use();

            // Color of Quad
            glUniform4f(glGetUniformLocation(shader->ID, "uColor"), color.r, color.g, color.b, color.a);
            // Size of Quad
            glUniform2f(glGetUniformLocation(shader->ID, "uSize"),
                (rect.width / SCREEN_WIDTH) * 2.0f, (rect.height / SCREEN_HEIGHT) * 2.0f);
            // Position of Quad
            glUniform2f(glGetUniformLocation(shader->ID, "uPosition"),
                ((rect.xPos + rect.width * 0.5f) / SCREEN_WIDTH) * 2.0f - 1.0f,
                1.0f - ((rect.yPos + rect.height * 0.5f) / SCREEN_HEIGHT) * 2.0f);
            // Rotation of Quad
            glUniform1f(glGetUniformLocation(shader->ID, "uRotation"), rect.rotation);
            // Tell the shader whether to sample tex0 or just use the flat color
            glUniform1i(glGetUniformLocation(shader->ID, "uUseTexture"), texture ? 1 : 0);

            if (texture) texture->Bind();

            vao->Bind();
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            vao->Unbind();

            if (texture) texture->Unbind();
        }
        // ===[Shared drag math for sliders]===
        float SliderNormalizedDragValue(const void* widgetId, Rect SliderRect, float handleWidth)
        {
            bool hoveringTrack = (mousePositionX >= SliderRect.xPos && mousePositionX <= SliderRect.xPos + SliderRect.width &&
                                    mousePositionY >= SliderRect.yPos && mousePositionY <= SliderRect.yPos + SliderRect.height);

            // Claim the drag on the exact frame the click lands on the track
            if (activeDragWidget == nullptr && leftMouseButtonPressed && hoveringTrack)
                activeDragWidget = widgetId;

            // Only the widget that owns the drag responds, and only while the button stays held
            if (activeDragWidget != widgetId || !leftMouseButtonDown) return -1.0f;

            float normalizedDragPosition = (float)(mousePositionX - SliderRect.xPos - handleWidth * 0.5f) / (SliderRect.width - handleWidth);
            return std::clamp(normalizedDragPosition, 0.0f, 1.0f);
        }

        // Stack of scissor boxes so nested scissors restore properly
        std::vector<std::array<GLint,4>> scissorStack;
        std::vector<GLboolean> scissorEnabledStack;
        // ===[Push a scissor rect, remembering whatever was active before]===
        void PushScissor(Rect rect)
        {
            GLboolean wasEnabled = glIsEnabled(GL_SCISSOR_TEST);
            GLint prev[4];
            glGetIntegerv(GL_SCISSOR_BOX, prev);

            scissorEnabledStack.push_back(wasEnabled);
            scissorStack.push_back({ prev[0], prev[1], prev[2], prev[3] });

            GLint scissorX = (GLint)(rect.xPos * SCREEN_SCALE_X);
            GLint scissorY = (GLint)((SCREEN_HEIGHT - (rect.yPos + rect.height)) * SCREEN_SCALE_Y);
            GLsizei scissorWidth = (GLsizei)(rect.width  * SCREEN_SCALE_X);
            GLsizei scissorHeight = (GLsizei)(rect.height * SCREEN_SCALE_Y);

            glEnable(GL_SCISSOR_TEST);
            glScissor(scissorX, scissorY, scissorWidth, scissorHeight);
        }
        // ===[Restore whatever scissor state was active before the matching PushScissor]===
        void PopScissor()
        {
            if (scissorStack.empty()) return;

            auto& prev = scissorStack.back();
            glScissor(prev[0], prev[1], prev[2], prev[3]);
            if (!scissorEnabledStack.back()) glDisable(GL_SCISSOR_TEST);

            scissorStack.pop_back();
            scissorEnabledStack.pop_back();
        }

        // ===[Shared shader/uniform setup + draw call for any piece of text]===
        void DrawTextRaw(float textPositionX, float bottomUpY, Color color, float size, const std::string& text, Font* font, float angleRadians)
        {
            if (!font) return;

            textShader->Use();

            // Set up an orthographic projection matching the current screen size
            glm::mat4 projection = glm::ortho(0.0f, SCREEN_WIDTH, 0.0f, SCREEN_HEIGHT);
            textShader->setMat4("projection", projection);
            glUniform4f(glGetUniformLocation(textShader->ID, "textColor"),
                color.r, color.g, color.b, color.a);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            font->rendererText(*textShader, text, textPositionX, bottomUpY, size, textVAO, textVBO, angleRadians);
        }
    }

    // |=====================================================
    // |---[Public Helper Functions]-------------------------
    // |=====================================================
    void SetMouseState(double mouseX, double mouseY,
        bool leftDown, bool leftPressed, bool leftReleased,
        bool rightDown, bool rightPressed, bool rightReleased,
        bool middleDown, bool middlePressed, bool middleReleased,
        double scrollDeltaX, double scrollDeltaY)
    {
        mousePositionX = mouseX;
        mousePositionY = mouseY;
        leftMouseButtonDown = leftDown;
        leftMouseButtonPressed = leftPressed;
        leftMouseButtonReleased = leftReleased;
        rightMouseButtonDown = rightDown;
        rightMouseButtonPressed = rightPressed;
        rightMouseButtonReleased = rightReleased;
        middleMouseButtonDown = middleDown;
        middleMouseButtonPressed = middlePressed;
        middleMouseButtonReleased = middleReleased;

        scrollDeltaXState = scrollDeltaX;
        scrollDeltaYState = scrollDeltaY;

        // A release always clears the active drag
        if (leftMouseButtonReleased || rightMouseButtonReleased || middleMouseButtonReleased)
            activeDragWidget = nullptr;
    }
    void GetScrollDelta(double& outX, double& outY)
    {
        outX = scrollDeltaXState;
        outY = scrollDeltaYState;
    }
    void SetKeyState(bool leftArrowPressed, bool rightArrowPressed, bool upArrowPressed, bool downArrowPressed)
    {
        leftArrowKeyPressed = leftArrowPressed;
        rightArrowKeyPressed = rightArrowPressed;
        upArrowKeyPressed = upArrowPressed;
        downArrowKeyPressed = downArrowPressed;
    }

    // |=====================================================
    // |---[Set window backend/Get graphics info]------------
    // |=====================================================
    void SetWindowBackend(WindowBackend backend)
    {
        graphicsInfo.backend = backend;
    }
    const GraphicsInfo& GetGraphicsInfo()
    {
        return graphicsInfo;
    }

    // |=====================================================
    // |---[Initlize stuff]----------------------------------
    // |=====================================================
    void init()
    {
        // ===[Query the live GL context, must run after glad is loaded]===
        glGetIntegerv(GL_MAJOR_VERSION, &graphicsInfo.glMajor);
        glGetIntegerv(GL_MINOR_VERSION, &graphicsInfo.glMinor);
        const char* version = (const char*)glGetString(GL_VERSION);
        const char* vendor = (const char*)glGetString(GL_VENDOR);
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        graphicsInfo.glVersionString = version ? version : "unknown";
        graphicsInfo.glVendor = vendor ? vendor : "unknown";
        graphicsInfo.glRenderer = renderer ? renderer : "unknown";

        // ===[Generates shader object using vertex and fragment shaders .glsl files]===
        shader = new Shader("../shader_files/VertexShader.glsl", "../shader_files/FragmentShader.glsl");

       	// Generates Vertex Array Object and binds it
       	vao = new VAO();
       	vao->Bind();

       	// Generates Vertex Buffer Object and links it to vertices
       	vbo = new VBO(rectangleVertices, sizeof(rectangleVertices));
       	// Generates Element Buffer Object and links it to indices
       	ebo = new EBO(rectangleIndices, sizeof(rectangleIndices));

       	// Links VBO attributes such as coordinates and colors to VAO
       	vao->LinkAttrib(*vbo, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
       	vao->LinkAttrib(*vbo, 1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
       	vao->LinkAttrib(*vbo, 2, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));
       	// Unbind all to prevent accidentally modifying them
       	vao->Unbind();
       	vbo->Unbind();
       	ebo->Unbind();

       	// Set the scale uniform once after shader is ready
        shader->Use();
        GLuint scale = glGetUniformLocation(shader->ID, "scale");
        glUniform1f(scale, 1.0f);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        InitTextRendering();
    }
    void InitTextRendering()
    {
        // Separate shader and VAO/VBO pair dedicated to glyph quads
        textShader = new Shader("../shader_files/VertexText.glsl", "../shader_files/FragmentText.glsl");

        glGenVertexArrays(1, &textVAO);
        glGenBuffers(1, &textVBO);
        glBindVertexArray(textVAO);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        // Allocate a dynamic buffer sized for one quad (6 verts, 4 floats each), refilled per glyph
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // |=====================================================
    // |---[Begin and End Panel]-----------------------------
    // |=====================================================
    void BeginPanel(Rect PanelRect, Color PanelColor, std::string PanelImagePath)
    {
        panelOpen = true;

        // Draw the panel background, optionally textured
        GLTexture* tex = PanelImagePath.empty() ? nullptr : GetOrLoadTexture(PanelImagePath);
        DrawQuad(PanelRect, PanelColor, nullptr);

        PushScissor(PanelRect);
    }
    void EndPanel()
    {
        panelOpen = false;
        PopScissor();
    }

    // |=====================================================
    // |---[Common UI stuff]---------------------------------
    // |=====================================================
    bool Button(Rect ButtonRect, Color ButtonColor, Color ButtonHoverColor, Color ButtonClickedColor, Color ButtonTextColor, float ButtonTextSize, std::string ButtonTextItself, std::string ButtonImagePath, std::string ButtonFontPath)
    {
        if (!panelOpen) return false;

        // Find out what state the button is on
        bool hovered = (mousePositionX >= ButtonRect.xPos && mousePositionX <= ButtonRect.xPos + ButtonRect.width &&
                        mousePositionY >= ButtonRect.yPos && mousePositionY <= ButtonRect.yPos + ButtonRect.height);
        bool clicked = hovered && leftMouseButtonPressed;

        // Pick color based on state
        Color finalColor = ButtonColor;
        if (hovered)
            finalColor = ButtonHoverColor;
        if (clicked)
            finalColor = ButtonClickedColor;

        // Draw button with the colors
        GLTexture* tex = GetOrLoadTexture(ButtonImagePath);
        DrawQuad(ButtonRect, finalColor, tex);

        // If it's empty and don't do shit
        if (!ButtonTextItself.empty())
        {
            Font* font = GetOrLoadFont(ButtonFontPath, (unsigned int)ButtonTextSize);
            if (font)
            {
                // Draw font but cut it off if it's too long and out of the button
                PushScissor(ButtonRect);

                // Center the label horizontally and roughly vertically inside the button
                float textWidth = font->measureTextWidth(ButtonTextItself, ButtonTextSize);
                float textHeight = font->measureTextHeight(ButtonTextItself, ButtonTextSize);

                float textX = ButtonRect.xPos + (ButtonRect.width - textWidth) * 0.5f;
                float centerYTopDown = ButtonRect.yPos + ButtonRect.height * 0.5f + textHeight * 0.5f;
                float bottomUpY = SCREEN_HEIGHT - centerYTopDown;

                DrawTextRaw(textX, bottomUpY, ButtonTextColor, ButtonTextSize, ButtonTextItself, font, 0.0f);

                PopScissor();
            }
        }

        return clicked;
    }
    bool IntSlider(Rect IntSliderRect, Color IntTrackColor, Color IntHandleColor, int& value, int minIntValue, int maxIntValue, int intStep, Color IntSliderTextColor, float IntSliderTextSize, std::string IntSliderTextItself, std::string IntSliderImagePath, std::string IntSliderFontPath)
    {
        if (!panelOpen) return false;

        // ===[Apply a drag, snapping the result to the nearest step]===
        bool changed = false;
        float handleWidth = IntSliderRect.height;
        float newNormalizedValue = SliderNormalizedDragValue(&value, IntSliderRect, handleWidth);
        if (newNormalizedValue >= 0.0f)
        {
            int steps = (maxIntValue - minIntValue) / std::max(intStep, 1);
            int newValue = minIntValue + (int)std::round(newNormalizedValue * steps) * intStep;
            newValue = std::clamp(newValue, minIntValue, maxIntValue);
            if (newValue != value)
            {
                value = newValue;
                changed = true;
            }
        }

        // ===[Keyboard nudging while hovered]===
        bool hoveredTrack = (mousePositionX >= IntSliderRect.xPos && mousePositionX <= IntSliderRect.xPos + IntSliderRect.width &&
                              mousePositionY >= IntSliderRect.yPos && mousePositionY <= IntSliderRect.yPos + IntSliderRect.height);

        if (hoveredTrack)
        {
            if (leftArrowKeyPressed || downArrowKeyPressed)
            {
                int newValue = std::clamp(value - intStep, minIntValue, maxIntValue);
                if (newValue != value) { value = newValue; changed = true; }
            }
            if (rightArrowKeyPressed || upArrowKeyPressed)
            {
                int newValue = std::clamp(value + intStep, minIntValue, maxIntValue);
                if (newValue != value) { value = newValue; changed = true; }
            }
        }

        // ===[Compute the handle position, using the up-to-date value]===
        float normalizedValue = (float)(value - minIntValue) / (float)(maxIntValue - minIntValue);
        float handleX = IntSliderRect.xPos + normalizedValue * (IntSliderRect.width - handleWidth);
        Rect handleRect{ handleX, IntSliderRect.yPos, handleWidth, IntSliderRect.height, 0.0f };

        // ===[Draw optional texture, and both the Slider Handle and Track]===
        GLTexture* trackTex = IntSliderImagePath.empty() ? nullptr : GetOrLoadTexture(IntSliderImagePath);
        DrawQuad(IntSliderRect, IntTrackColor, nullptr);
        DrawQuad(handleRect, IntHandleColor, nullptr);

        // ===[Label centered on the track both horizontally and vertically]===
        if (!IntSliderTextItself.empty())
        {
            Font* font = GetOrLoadFont(IntSliderFontPath, (unsigned int)IntSliderTextSize);
            if (font)
            {
                PushScissor(IntSliderRect);

                float textWidth = font->measureTextWidth(IntSliderTextItself, IntSliderTextSize);
                float textHeight = font->measureTextHeight(IntSliderTextItself, IntSliderTextSize);

                float textX = IntSliderRect.xPos + (IntSliderRect.width - textWidth) * 0.5f;
                float centerYTopDown = IntSliderRect.yPos + IntSliderRect.height * 0.5f + textHeight * 0.5f;
                float bottomUpY = SCREEN_HEIGHT - centerYTopDown;

                DrawTextRaw(textX, bottomUpY, IntSliderTextColor, IntSliderTextSize, IntSliderTextItself, font, 0.0f);

                PopScissor();
            }
        }

        return changed;
    }
    bool FloatSlider(Rect FloatSliderRect, Color FloatTrackColor, Color FloatHandleColor, float& value, float minFloatValue, float maxFloatValue, Color FloatSliderTextColor, float FloatSliderTextSize, std::string FloatSliderTextItself, std::string FloatSliderImagePath, std::string FloatSliderFontPath)
    {
        if (!panelOpen) return false;

        // ===[Apply a drag directly as a continuous value, no stepping needed]===
        bool changed = false;
        float handleWidth = FloatSliderRect.height;
        float newNormalizedValue = SliderNormalizedDragValue(&value, FloatSliderRect, handleWidth);
        if (newNormalizedValue >= 0.0f)
        {
            float newValue = minFloatValue + newNormalizedValue * (maxFloatValue - minFloatValue);
            if (newValue != value)
            {
                value = newValue;
                changed = true;
            }
        }

        // ===[Keyboard nudging while hovered]===
        bool hoveredTrack = (mousePositionX >= FloatSliderRect.xPos && mousePositionX <= FloatSliderRect.xPos + FloatSliderRect.width &&
                              mousePositionY >= FloatSliderRect.yPos && mousePositionY <= FloatSliderRect.yPos + FloatSliderRect.height);

        if (hoveredTrack)
        {
            float step = (maxFloatValue - minFloatValue) * 0.01f; // 1% per keypress
            if (leftArrowKeyPressed || downArrowKeyPressed)
            {
                float newValue = std::clamp(value - step, minFloatValue, maxFloatValue);
                if (newValue != value) { value = newValue; changed = true; }
            }
            if (rightArrowKeyPressed || upArrowKeyPressed)
            {
                float newValue = std::clamp(value + step, minFloatValue, maxFloatValue);
                if (newValue != value) { value = newValue; changed = true; }
            }
        }

        // ===[Compute the handle position, using the up-to-date value]===
        float normalizedValue = (value - minFloatValue) / (maxFloatValue - minFloatValue);
        float handleX = FloatSliderRect.xPos + normalizedValue * (FloatSliderRect.width - handleWidth);
        Rect handleRect{ handleX, FloatSliderRect.yPos, handleWidth, FloatSliderRect.height, 0.0f };

        // ===[Draw optional texture, and both the Slider Handle and Track]===
        GLTexture* trackTex = FloatSliderImagePath.empty() ? nullptr : GetOrLoadTexture(FloatSliderImagePath);
        DrawQuad(FloatSliderRect, FloatTrackColor, nullptr);
        DrawQuad(handleRect, FloatHandleColor, nullptr);

        // ===[Label centered on the track both horizontally and vertically]===
        if (!FloatSliderTextItself.empty())
        {
            Font* font = GetOrLoadFont(FloatSliderFontPath, (unsigned int)FloatSliderTextSize);
            if (font)
            {
                PushScissor(FloatSliderRect);

                float textWidth = font->measureTextWidth(FloatSliderTextItself, FloatSliderTextSize);
                float textHeight = font->measureTextHeight(FloatSliderTextItself, FloatSliderTextSize);

                float textX = FloatSliderRect.xPos + (FloatSliderRect.width - textWidth) * 0.5f;
                float centerYTopDown = FloatSliderRect.yPos + FloatSliderRect.height * 0.5f + textHeight * 0.5f;
                float bottomUpY = SCREEN_HEIGHT - centerYTopDown;

                DrawTextRaw(textX, bottomUpY, FloatSliderTextColor, FloatSliderTextSize, FloatSliderTextItself, font, 0.0f);

                PopScissor();
            }
        }

        return changed;
    }
    bool Switch(Rect SwitchRect, Color SwitchOnColor, Color SwitchOffColor, Color SwitchTextColor, bool& value, float SwitchTextSize, std::string SwitchOnTextItself, std::string SwitchOffTextItself, std::string SwitchImagePath, std::string SwitchFontPath)
    {
        if (!panelOpen) return false;

        bool hovered = (mousePositionX >= SwitchRect.xPos && mousePositionX <= SwitchRect.xPos + SwitchRect.width &&
                        mousePositionY >= SwitchRect.yPos && mousePositionY <= SwitchRect.yPos + SwitchRect.height);

        // A click anywhere on the switch flips its boolean state
        bool toggled = false;
        if (hovered && leftMouseButtonPressed)
        {
            value = !value;
            toggled = true;
        }

        // ===[Draw optional texture and quad]===
        GLTexture* tex = SwitchImagePath.empty() ? nullptr : GetOrLoadTexture(SwitchImagePath);
        DrawQuad(SwitchRect, value ? SwitchOnColor : SwitchOffColor, nullptr);

        // Pick the label based on current state
        const std::string& activeText = value ? SwitchOnTextItself : SwitchOffTextItself;
        if (!activeText.empty())
        {
            Font* font = GetOrLoadFont(SwitchFontPath, (unsigned int)SwitchTextSize);
            if (font)
            {
                PushScissor(SwitchRect);

                // Center the active label both horizontally and vertically
                float textWidth = font->measureTextWidth(activeText, SwitchTextSize);
                float textHeight = font->measureTextHeight(activeText, SwitchTextSize);

                float textX = SwitchRect.xPos + (SwitchRect.width - textWidth) * 0.5f;
                float centerYTopDown = SwitchRect.yPos + SwitchRect.height * 0.5f + textHeight * 0.5f;
                float bottomUpY = SCREEN_HEIGHT - centerYTopDown;

                DrawTextRaw(textX, bottomUpY, SwitchTextColor, SwitchTextSize, activeText, font, 0.0f);

                PopScissor();
            }
        }

        return toggled;
    }

    // |=====================================================
    // |---[UnCommon UI stuff]-------------------------------
    // |=====================================================
    void Image(Rect ImageRect, Color ImageColor, std::string ImagePath)
    {
        if (!panelOpen) return;

        GLTexture* tex = GetOrLoadTexture(ImagePath);
        DrawQuad(ImageRect, ImageColor, tex);
    }
    void Separator(Rect SeparatorRect, Color SeparatorColor)
    {
        if (!panelOpen) return;

        DrawQuad(SeparatorRect, SeparatorColor, nullptr);
    }
    void Text(Rect TextRect, Color TextColor, float TextSize, std::string TextItself, std::string FontPath)
    {
        if (!panelOpen) return;

        Font* font = GetOrLoadFont(FontPath,(unsigned int)TextSize);
        if (!font) return;

        // Convert top-down Rect.yPos into the bottom-up space the glyph math/projection expect
        float bottomUpY = SCREEN_HEIGHT - TextRect.yPos;
        // Covert Rectangles degrees to radians
        float angleRadians = glm::radians(TextRect.rotation);

        font->rendererText(*textShader, TextItself, TextRect.xPos, bottomUpY, TextSize, textVAO, textVBO, angleRadians);
    }

    // |=====================================================
    // |---[ALL OUR FOOD KEEPS BLOWING UP]-------------------
    // |=====================================================
    void BlowUp()
    {
        // ===[Free every cached texture]===
        for (auto& [path, tex] : textureCache)
        {
            tex->Delete();
            delete tex;
        }
        textureCache.clear();

        // ===[Free every cached font]===
        for (auto& [path, font] : fontCache)
        {
            if (!font) continue;
            font->deleteFreeType();
            delete font;
        }
        fontCache.clear();
    }
}
