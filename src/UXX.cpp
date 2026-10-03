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

        GLint uColorLoc = -1,
            uSizeLoc = -1,
            uPositionLoc = -1,
            uRotationLoc = -1,
            uUseTextureLoc = -1,
            uDarkenLoc = -1;
        GLint textColorLoc = -1;
        GLint textModelLoc = -1;

        // ===[Caches]===
        std::unordered_map<std::string, GLTexture*> textureCache;
        GLTexture* GetOrLoadTexture(const std::string& path);

        std::unordered_map<std::string, Font*> fontCache;
        Font* GetOrLoadFont(const std::string& path, unsigned int pixelHeight = 48);

        // Forward declaration needed because SliderNormalizedDragValue calls WidgetClaimsActive
        // before its own definition appears in this file
        bool WidgetClaimsActive(const void* widgetIdentifier, bool widgetIsHoveredThisFrame);

        // ===[Tile]===
        bool panelOpen = false;
        // ===[Keyboard]===
        bool leftArrowKeyPressed = false;
        bool rightArrowKeyPressed = false;
        bool upArrowKeyPressed = false;
        bool downArrowKeyPressed = false;

        // ===[Shared hover/active widget ownership, used by every widget type]===
        const void* currentFrameHoveredWidgetIdentifier = nullptr;
        const void* currentFrameActiveWidgetIdentifier = nullptr;

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

        bool mouseHoveredOverWidgetThisFrame = false;

        double mousePositionX = -1.0, mousePositionY = -1.0;
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
            auto foundTextureEntry = textureCache.find(path);
            if (foundTextureEntry != textureCache.end()) return foundTextureEntry->second;

            GLTexture* loadedTexture = nullptr;
            try
            {
                loadedTexture = new GLTexture(path.c_str(), "", 0);
                loadedTexture->texUnit(*shader, "tex0", 0);
            }
            catch (const std::exception& e)
            {
                std::cerr << "[UXX] Texture completely unavailable for \"" << path
                            << "\": " << e.what() << " - drawing flat color instead.\n";
                delete loadedTexture;
                loadedTexture = nullptr;
            }

            textureCache[path] = loadedTexture;
            return loadedTexture;
        }
        // ===[foot]===
        Font* GetOrLoadFont(const std::string& path, unsigned int pixelHeight)
        {
            // Key by path+size so the same font at different sizes stays distinct
            std::string key = path + "#" + std::to_string(pixelHeight);

            auto foundFontEntry = fontCache.find(key);
            if (foundFontEntry != fontCache.end()) return foundFontEntry->second;

            Font* font = new Font();
            if (!font->initFreeType(path.c_str(), pixelHeight))
            {
                std::cerr << "Failed to load font: " << path << "\n";
                delete font;
                fontCache[key] = nullptr;
                return nullptr;
            }

            fontCache[key] = font;
            return font;
        }
        // ===[Multiply just RGB by a factor, used to darken flat non-textured widget
        // colors on hover/press alpha is left alone]===
        Color ApplyDarken(Color color, float darkenFactor)
        {
            return Color(color.red * darkenFactor, color.green * darkenFactor, color.blue * darkenFactor, color.alpha);
        }
        // Shared hover/active darken factors so every widget dims consistently
        constexpr float kHoverDarkenFactor  = 0.85f;
        constexpr float kActiveDarkenFactor = 0.70f;
        // ===[Be there or be SQUARE]===
        void DrawQuad(Rect rect, Color color, GLTexture* texture, float darkenAmount = 1.0f)
        {
            shader->Use();

            // Color of Quad
            glUniform4f(uColorLoc, color.red, color.green, color.blue, color.alpha);
            // Size of Quad
            glUniform2f(uSizeLoc, (rect.width / SCREEN_WIDTH) * 2.0f, (rect.height / SCREEN_HEIGHT) * 2.0f);
            // Position of Quad
            glUniform2f(uPositionLoc,
                ((rect.xPos + rect.width * 0.5f) / SCREEN_WIDTH) * 2.0f - 1.0f,
                1.0f - ((rect.yPos + rect.height * 0.5f) / SCREEN_HEIGHT) * 2.0f);
            // Rotation of Quad
            glUniform1f(uRotationLoc, rect.rotation);
            // Tell the shader whether to sample tex0 or just use the flat color
            glUniform1i(uUseTextureLoc, texture ? 1 : 0);
            // Darken of Quad when hoverd/clicked
            glUniform1f(uDarkenLoc, darkenAmount);

            if (texture) texture->Bind();
            vao->Bind();
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            vao->Unbind();
            if (texture) texture->Unbind();
        }
        // ===[Shared shader/uniform setup + draw call for any piece of text]===
        void DrawTextRaw(float textPositionX, float bottomUpY, Color color, float size, const std::string& text, Font* font, float angleRadians)
        {
            if (!font) return;

            textShader->Use();

            // Set up an orthographic projection matching the current screen size
            glm::mat4 projection = glm::ortho(0.0f, SCREEN_WIDTH, 0.0f, SCREEN_HEIGHT);
            textShader->setMat4("projection", projection);
            glUniform4f(textColorLoc, color.red, color.green, color.blue, color.alpha);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            font->rendererText(*textShader, text, textPositionX, bottomUpY, size, textVAO, textVBO, textModelLoc, angleRadians);
        }
        // ===[Shared drag math for sliders]===
        float SliderNormalizedDragValue(const void* widgetIdentifier, Rect sliderRect, float handleWidth, bool widgetIsHoveredThisFrame)
        {
            bool widgetIsActive = WidgetClaimsActive(widgetIdentifier, widgetIsHoveredThisFrame);

            // Only the widget that owns the drag responds, and only while the button stays held
            if (!widgetIsActive || !leftMouseButtonDown) return -1.0f;

            float normalizedDragPosition = (float)(mousePositionX - sliderRect.xPos - handleWidth * 0.5f) / (sliderRect.width - handleWidth);
            return std::clamp(normalizedDragPosition, 0.0f, 1.0f);
        }
        // ===[Claims hover for whichever widget is topmost under the cursor this frame]===
        bool WidgetIsHovered(const void* widgetIdentifier, Rect widgetBounds)
        {
            bool cursorInsideBounds = (mousePositionX >= widgetBounds.xPos && mousePositionX <= widgetBounds.xPos + widgetBounds.width &&
                                        mousePositionY >= widgetBounds.yPos && mousePositionY <= widgetBounds.yPos + widgetBounds.height);

            // Last widget submitted this frame that contains the cursor wins hover,
            if (cursorInsideBounds)
                currentFrameHoveredWidgetIdentifier = widgetIdentifier;

            return currentFrameHoveredWidgetIdentifier == widgetIdentifier;
        }

        // ===[Claims the active slot on press, holds it until release, shared by every widget type]===
        bool WidgetClaimsActive(const void* widgetIdentifier, bool widgetIsHoveredThisFrame)
        {
            if (currentFrameActiveWidgetIdentifier == nullptr && widgetIsHoveredThisFrame && leftMouseButtonPressed)
                currentFrameActiveWidgetIdentifier = widgetIdentifier;

            return currentFrameActiveWidgetIdentifier == widgetIdentifier;
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
        // Text typed this frame, delivered by the backend
        std::string typedCharactersThisFrame;
        // Caret position inside each input field, keyed by the widget's value address
        std::unordered_map<const void*, size_t> caretIndexMap;

        // Editing key presses counted this frame, delivered by the backend
        int backspacePressCountThisFrame = 0;
        int enterPressCountThisFrame = 0;
        int leftArrowPressCountThisFrame = 0;
        int rightArrowPressCountThisFrame = 0;
        int upArrowPressCountThisFrame = 0;
        int downArrowPressCountThisFrame = 0;
        int homePressCountThisFrame = 0;
        int endPressCountThisFrame = 0;
        int deletePressCountThisFrame = 0;

        // Which input field currently receives typing (nullptr means none)
        const void* focusedInputFieldIdentifier = nullptr;

        // Number fields edit a text buffer while focused, keyed by the widget's value address
        std::unordered_map<const void*, std::string> numberFieldEditBufferMap;

        // Click inside to focus (caret goes to the end), click outside to unfocus
        bool UpdateInputFieldFocus(const void* widgetIdentifier, bool widgetIsHoveredThisFrame)
        {
            if (leftMouseButtonPressed)
            {
                if (widgetIsHoveredThisFrame)
                {
                    // npos gets clamped to the text length later, which means "the end"
                    if (focusedInputFieldIdentifier != widgetIdentifier)
                        caretIndexMap[widgetIdentifier] = std::string::npos;

                    focusedInputFieldIdentifier = widgetIdentifier;
                }
                else if (focusedInputFieldIdentifier == widgetIdentifier)
                {
                    focusedInputFieldIdentifier = nullptr;
                }
            }
            return focusedInputFieldIdentifier == widgetIdentifier;
        }
        // Index where the line holding the caret begins
        size_t FindLineStartIndex(const std::string& textToSearch, size_t caretIndex)
        {
            if (caretIndex == 0) return 0;

            size_t previousNewlineIndex = textToSearch.rfind('\n', caretIndex - 1);
            return previousNewlineIndex == std::string::npos ? 0 : previousNewlineIndex + 1;
        }
        // Index where the line holding the caret ends (its newline, or the end of the text)
        size_t FindLineEndIndex(const std::string& textToSearch, size_t caretIndex)
        {
            size_t nextNewlineIndex = textToSearch.find('\n', caretIndex);
            return nextNewlineIndex == std::string::npos ? textToSearch.size() : nextNewlineIndex;
        }
        // Moves the caret up one line, keeping the same column when possible
        void MoveCaretUpOneLine(const std::string& textToSearch, size_t& caretIndex)
        {
            size_t currentLineStartIndex = FindLineStartIndex(textToSearch, caretIndex);
            if (currentLineStartIndex == 0) return;

            size_t columnIndex = caretIndex - currentLineStartIndex;
            size_t previousLineStartIndex = FindLineStartIndex(textToSearch, currentLineStartIndex - 1);
            size_t previousLineLength = (currentLineStartIndex - 1) - previousLineStartIndex;

            caretIndex = previousLineStartIndex + std::min(columnIndex, previousLineLength);
        }
        // Moves the caret down one line, keeping the same column when possible
        void MoveCaretDownOneLine(const std::string& textToSearch, size_t& caretIndex)
        {
            size_t currentLineEndIndex = FindLineEndIndex(textToSearch, caretIndex);
            if (currentLineEndIndex >= textToSearch.size()) return;

            size_t columnIndex = caretIndex - FindLineStartIndex(textToSearch, caretIndex);
            size_t nextLineStartIndex = currentLineEndIndex + 1;
            size_t nextLineLength = FindLineEndIndex(textToSearch, nextLineStartIndex) - nextLineStartIndex;

            caretIndex = nextLineStartIndex + std::min(columnIndex, nextLineLength);
        }
        // Applies this frame's caret movement and typing, returns true if the text changed
        bool ApplyTypingToText(std::string& textToEdit, size_t& caretIndex, bool allowNewlines, const std::string& allowedCharacters)
        {
            bool textChanged = false;
            caretIndex = std::min(caretIndex, textToEdit.size());

            // Left and right arrows move one character at a time
            for (int pressIndex = 0; pressIndex < leftArrowPressCountThisFrame; pressIndex++)
                if (caretIndex > 0) caretIndex--;
            for (int pressIndex = 0; pressIndex < rightArrowPressCountThisFrame; pressIndex++)
                if (caretIndex < textToEdit.size()) caretIndex++;

            // Up and down arrows only make sense in multi-line fields
            if (allowNewlines)
            {
                for (int pressIndex = 0; pressIndex < upArrowPressCountThisFrame; pressIndex++)
                    MoveCaretUpOneLine(textToEdit, caretIndex);
                for (int pressIndex = 0; pressIndex < downArrowPressCountThisFrame; pressIndex++)
                    MoveCaretDownOneLine(textToEdit, caretIndex);
            }

            // Home and End jump to the start or end of the current line
            if (homePressCountThisFrame > 0) caretIndex = FindLineStartIndex(textToEdit, caretIndex);
            if (endPressCountThisFrame > 0) caretIndex = FindLineEndIndex(textToEdit, caretIndex);

            // Insert typed characters at the caret, skipping any that are not allowed
            for (char typedCharacter : typedCharactersThisFrame)
            {
                if (!allowedCharacters.empty() && allowedCharacters.find(typedCharacter) == std::string::npos) continue;
                textToEdit.insert(caretIndex, 1, typedCharacter);
                caretIndex++;
                textChanged = true;
            }

            // Backspace removes the character before the caret
            for (int pressIndex = 0; pressIndex < backspacePressCountThisFrame; pressIndex++)
            {
                if (caretIndex == 0) break;
                textToEdit.erase(caretIndex - 1, 1);
                caretIndex--;
                textChanged = true;
            }

            // Delete removes the character after the caret
            for (int pressIndex = 0; pressIndex < deletePressCountThisFrame; pressIndex++)
            {
                if (caretIndex >= textToEdit.size()) break;
                textToEdit.erase(caretIndex, 1);
                textChanged = true;
            }

            // Enter adds a new line at the caret in multi-line fields, otherwise it ends editing
            for (int pressIndex = 0; pressIndex < enterPressCountThisFrame; pressIndex++)
            {
                if (allowNewlines)
                {
                    textToEdit.insert(caretIndex, 1, '\n');
                    caretIndex++;
                    textChanged = true;
                }
                else focusedInputFieldIdentifier = nullptr;
            }

            return textChanged;
        }
        // Picks the bracket color for a character, or the normal text color
        Color PickCodeCharacterColor(char character, const CodeInputFieldStyle& codeStyle)
        {
            if (character == '(' || character == ')') return codeStyle.roundBracketColor;
            if (character == '[' || character == ']') return codeStyle.squareBracketColor;
            if (character == '{' || character == '}') return codeStyle.curlyBracketColor;
            if (character == '<' || character == '>') return codeStyle.angleBracketColor;
            return codeStyle.textColor;
        }
        // Draws the text line by line plus a blinking caret, colors brackets if codeStyle is given
        void DrawInputFieldText(Rect fieldRect, const std::string& textToDraw, size_t caretIndex, bool fieldIsFocused,
            Color textColor, float textSize, const std::string& fontPath,
            const CodeInputFieldStyle* codeStyle)
        {
            Font* font = GetOrLoadFont(fontPath, (unsigned int)textSize);
            if (!font) return;

            PushScissor(fieldRect);

            const float paddingInsideField = 8.0f;
            float lineHeight = font->measureTextHeight("A", textSize) * 1.5f;
            caretIndex = std::min(caretIndex, textToDraw.size());

            size_t lineStartIndex = 0;
            int lineNumber = 0;
            int caretLineNumber = 0;
            std::string textBeforeCaretOnItsLine;

            // Walk through the text one line at a time
            while (true)
            {
                size_t lineEndIndex = textToDraw.find('\n', lineStartIndex);
                std::string lineText = textToDraw.substr(lineStartIndex,
                    lineEndIndex == std::string::npos ? std::string::npos : lineEndIndex - lineStartIndex);

                float baselineTopDownY = fieldRect.yPos + paddingInsideField + lineHeight * (lineNumber + 1);
                float baselineBottomUpY = SCREEN_HEIGHT - baselineTopDownY;
                float lineStartX = fieldRect.xPos + paddingInsideField;

                if (codeStyle)
                {
                    // Draw runs of normal text in one call, and each bracket on its own for its color
                    const std::string bracketCharacters = "()[]{}<>";
                    float currentRunStartX = lineStartX;
                    size_t runStartIndex = 0;

                    while (runStartIndex < lineText.size())
                    {
                        bool runIsBracket = bracketCharacters.find(lineText[runStartIndex]) != std::string::npos;
                        size_t runEndIndex = runStartIndex + 1;

                        // Normal text keeps growing until the next bracket, a bracket is always one character
                        if (!runIsBracket)
                        {
                            while (runEndIndex < lineText.size() &&
                                   bracketCharacters.find(lineText[runEndIndex]) == std::string::npos)
                                runEndIndex++;
                        }

                        std::string runText = lineText.substr(runStartIndex, runEndIndex - runStartIndex);
                        DrawTextRaw(currentRunStartX, baselineBottomUpY,
                                    PickCodeCharacterColor(lineText[runStartIndex], *codeStyle),
                                    textSize, runText, font, 0.0f);

                        // Move the start of the next run to the right by the width of this run
                        currentRunStartX += font->measureTextWidth(runText, textSize);
                        runStartIndex = runEndIndex;
                    }
                }
                else
                    DrawTextRaw(lineStartX, baselineBottomUpY, textColor, textSize, lineText, font, 0.0f);

                // Remember which line holds the caret and the text to the left of it
                if (caretIndex >= lineStartIndex && caretIndex <= lineStartIndex + lineText.size())
                {
                    caretLineNumber = lineNumber;
                    textBeforeCaretOnItsLine = lineText.substr(0, caretIndex - lineStartIndex);
                }

                if (lineEndIndex == std::string::npos) break;
                lineStartIndex = lineEndIndex + 1;
                lineNumber++;
            }

            // Blink the caret at its real position while focused
            bool caretIsVisible = fieldIsFocused && std::fmod(glfwGetTime(), 1.0) < 0.5;
            if (caretIsVisible)
            {
                float caretX = fieldRect.xPos + paddingInsideField + font->measureTextWidth(textBeforeCaretOnItsLine, textSize);
                float caretTopY = fieldRect.yPos + paddingInsideField + lineHeight * caretLineNumber;
                DrawQuad(Rect(caretX, caretTopY, 2.0f, lineHeight), textColor, nullptr);
            }

            PopScissor();
        }
        // Draws background, handles hover and focus, shared by all three fields
        bool BeginInputField(const void* widgetIdentifier, Rect fieldRect, Color backgroundColor,
                             const std::string& imagePath)
        {
            bool fieldIsHovered = WidgetIsHovered(widgetIdentifier, fieldRect);
            if (fieldIsHovered) mouseHoveredOverWidgetThisFrame = true;

            bool fieldIsFocused = UpdateInputFieldFocus(widgetIdentifier, fieldIsHovered);

            GLTexture* backgroundTexture = imagePath.empty() ? nullptr : GetOrLoadTexture(imagePath);
            DrawQuad(fieldRect, backgroundColor, backgroundTexture);

            return fieldIsFocused;
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

        mouseHoveredOverWidgetThisFrame = false;
        currentFrameHoveredWidgetIdentifier = nullptr;

        // A release always clears the active widget
        if (leftMouseButtonReleased || rightMouseButtonReleased || middleMouseButtonReleased)
            currentFrameActiveWidgetIdentifier = nullptr;
    }
    void GetScrollDelta(double& outX, double& outY)
    {
        outX = scrollDeltaXState;
        outY = scrollDeltaYState;
    }
    void GetMouse(double* x, double* y,
        bool* leftDown, bool* leftPressed, bool* leftReleased,
        bool* rightDown, bool* rightPressed, bool* rightReleased,
        bool* middleDown, bool* middlePressed, bool* middleReleased,
        double* scrollX, double* scrollY)
    {
        if (x) *x = mousePositionX;
        if (y) *y = mousePositionY;

        if (leftDown)     *leftDown     = leftMouseButtonDown;
        if (leftPressed)  *leftPressed  = leftMouseButtonPressed;
        if (leftReleased) *leftReleased = leftMouseButtonReleased;

        if (rightDown)     *rightDown     = rightMouseButtonDown;
        if (rightPressed)  *rightPressed  = rightMouseButtonPressed;
        if (rightReleased) *rightReleased = rightMouseButtonReleased;

        if (middleDown)     *middleDown     = middleMouseButtonDown;
        if (middlePressed)  *middlePressed  = middleMouseButtonPressed;
        if (middleReleased) *middleReleased = middleMouseButtonReleased;

        if (scrollX) *scrollX = scrollDeltaXState;
        if (scrollY) *scrollY = scrollDeltaYState;
    }
    void SetKeyState(bool leftArrowPressed, bool rightArrowPressed, bool upArrowPressed, bool downArrowPressed)
    {
        leftArrowKeyPressed = leftArrowPressed;
        rightArrowKeyPressed = rightArrowPressed;
        upArrowKeyPressed = upArrowPressed;
        downArrowKeyPressed = downArrowPressed;
    }
    bool MouseHoveredOverWidget()
    {
        return mouseHoveredOverWidgetThisFrame;
    }
    void SetTextInputState(const std::string& typedCharacters, int backspacePressCount, int enterPressCount)
    {
        typedCharactersThisFrame = typedCharacters;
        backspacePressCountThisFrame = backspacePressCount;
        enterPressCountThisFrame = enterPressCount;
    }
    // Receives how many times each editing key was pressed this frame
    void SetEditKeyState(int leftArrowPressCount, int rightArrowPressCount, int upArrowPressCount,
                         int downArrowPressCount, int homePressCount, int endPressCount, int deletePressCount)
    {
        leftArrowPressCountThisFrame = leftArrowPressCount;
        rightArrowPressCountThisFrame = rightArrowPressCount;
        upArrowPressCountThisFrame = upArrowPressCount;
        downArrowPressCountThisFrame = downArrowPressCount;
        homePressCountThisFrame = homePressCount;
        endPressCountThisFrame = endPressCount;
        deletePressCountThisFrame = deletePressCount;
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
        GLuint scale   = glGetUniformLocation(shader->ID, "scale");
        glUniform1f(scale, 1.0f);
        uColorLoc      = glGetUniformLocation(shader->ID, "uColor");
        uSizeLoc       = glGetUniformLocation(shader->ID, "uSize");
        uPositionLoc   = glGetUniformLocation(shader->ID, "uPosition");
        uRotationLoc   = glGetUniformLocation(shader->ID, "uRotation");
        uUseTextureLoc = glGetUniformLocation(shader->ID, "uUseTexture");
        uDarkenLoc     = glGetUniformLocation(shader->ID, "uDarken");

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        InitTextRendering();
    }
    void InitTextRendering()
    {
        // Separate shader and VAO/VBO pair dedicated to glyph quads
        textShader   = new Shader("../shader_files/VertexText.glsl", "../shader_files/FragmentText.glsl");
        textColorLoc = glGetUniformLocation(textShader->ID, "textColor");
        textModelLoc = glGetUniformLocation(textShader->ID, "model");

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
        GLTexture* panelTexture = PanelImagePath.empty() ? nullptr : GetOrLoadTexture(PanelImagePath);
        DrawQuad(PanelRect, PanelColor, panelTexture);

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
    bool Button(Rect ButtonRect, const ButtonStyle& buttonStyle, std::string ButtonTextItself)
    {
        if (!panelOpen) return false;

        Color ButtonColor           = buttonStyle.normalColor;
        Color ButtonHoverColor      = buttonStyle.hoveredColor;
        Color ButtonClickedColor    = buttonStyle.clickedColor;
        Color ButtonTextColor       = buttonStyle.textColor;
        float ButtonTextSize        = buttonStyle.textSize;
        std::string ButtonImagePath = buttonStyle.imagePath;
        std::string ButtonFontPath  = buttonStyle.fontPath;

        // Find out what state the button is on
        bool hoveredButton = (mousePositionX >= ButtonRect.xPos && mousePositionX <= ButtonRect.xPos + ButtonRect.width &&
                                mousePositionY >= ButtonRect.yPos && mousePositionY <= ButtonRect.yPos + ButtonRect.height);
        if (hoveredButton) mouseHoveredOverWidgetThisFrame = true;
        bool clickedButton = hoveredButton && leftMouseButtonPressed;

        // Pick color based on state
        Color finalColor = ButtonColor;
        float darkenAmount = 1.0f;
        if (hoveredButton)
        {
            finalColor = ButtonHoverColor;
            darkenAmount = kHoverDarkenFactor;
        }
        if (clickedButton)
        {
            finalColor = ButtonClickedColor;
            darkenAmount = kActiveDarkenFactor;
        }

        // Draw button with the colors
        GLTexture* buttonTexture = ButtonImagePath.empty() ? nullptr : GetOrLoadTexture(ButtonImagePath);
        DrawQuad(ButtonRect, finalColor, buttonTexture, darkenAmount);

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

        return clickedButton;
    }
    bool IntSlider(Rect IntSliderRect, int& value, int minIntValue, int maxIntValue, int intStep,
        const SliderStyle& sliderStyle, std::string IntSliderTextItself)
    {
        if (!panelOpen) return false;

        Color IntTrackColor                  = sliderStyle.trackColor;
        Color IntHandleColor                 = sliderStyle.handleColor;
        Color IntSliderTextColor             = sliderStyle.textColor;
        float IntSliderTextSize              = sliderStyle.textSize;
        std::string IntSliderTrackImagePath  = sliderStyle.trackImagePath;
        std::string IntSliderHandleImagePath = sliderStyle.handleImagePath;
        std::string IntSliderFontPath        = sliderStyle.fontPath;

        bool changed = false;
        float handleWidth = IntSliderRect.height;
        int safeIntStep = std::max(intStep, 1);

        // ===[Hover must be known before we ask whether this widget owns the drag]===
        bool hoveredIntTrack = WidgetIsHovered(&value, IntSliderRect);
        if (hoveredIntTrack) mouseHoveredOverWidgetThisFrame = true;

        // ===[Apply a drag, snapping the result to the nearest step]===
        float normalizedDragPositionThisFrame = SliderNormalizedDragValue(&value, IntSliderRect, handleWidth, hoveredIntTrack);
        if (normalizedDragPositionThisFrame >= 0.0f)
        {
            int totalSteps = (maxIntValue - minIntValue) / safeIntStep;
            int draggedValue = minIntValue + (int)std::round(normalizedDragPositionThisFrame * totalSteps) * safeIntStep;
            draggedValue = std::clamp(draggedValue, minIntValue, maxIntValue);
            if (draggedValue != value)
            {
                value = draggedValue;
                changed = true;
            }
        }

        // ===[Keyboard nudging while hovered]===
        if (hoveredIntTrack)
        {
            if (leftArrowKeyPressed || downArrowKeyPressed)
            {
                int nudgedValue = std::clamp(value - safeIntStep, minIntValue, maxIntValue);
                if (nudgedValue != value) { value = nudgedValue; changed = true; }
            }
            if (rightArrowKeyPressed || upArrowKeyPressed)
            {
                int nudgedValue = std::clamp(value + safeIntStep, minIntValue, maxIntValue);
                if (nudgedValue != value) { value = nudgedValue; changed = true; }
            }
        }

        // ===[Handle position always tracks the actual current value, not the transient drag delta,
        //     so it stays correct on frames where the slider isn't being actively dragged]===
        float currentNormalizedPosition = (maxIntValue != minIntValue)
            ? (float)(value - minIntValue) / (float)(maxIntValue - minIntValue)
            : 0.0f;
        float handleX = IntSliderRect.xPos + currentNormalizedPosition * (IntSliderRect.width - handleWidth);
        Rect handleRect{ handleX, IntSliderRect.yPos, handleWidth, IntSliderRect.height, 0.0f };

        // ===[Handle darkening: same has-image vs no-image split as Button]===
        bool isActiveIntSlider = (currentFrameActiveWidgetIdentifier == &value);
        float handleDarkenAmount = 1.0f;
        if (isActiveIntSlider)     handleDarkenAmount = kActiveDarkenFactor;
        else if (hoveredIntTrack)  handleDarkenAmount = kHoverDarkenFactor;

        // ===[Draw optional texture, and both the Slider Handle and Track]===
        GLTexture* intTrackTexture  = IntSliderTrackImagePath.empty()  ? nullptr : GetOrLoadTexture(IntSliderTrackImagePath);
        GLTexture* intHandleTexture = IntSliderHandleImagePath.empty() ? nullptr : GetOrLoadTexture(IntSliderHandleImagePath);
        DrawQuad(IntSliderRect, IntTrackColor, intTrackTexture);
        // No image -> pre-darken the flat handle color. Has image -> pass the darken
        // factor through and let the shader darken the texture instead.
        Color finalHandleColor = intHandleTexture ? IntHandleColor : ApplyDarken(IntHandleColor, handleDarkenAmount);
        DrawQuad(handleRect, finalHandleColor, intHandleTexture, handleDarkenAmount);

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
    bool FloatSlider(Rect FloatSliderRect, float& value, float minFloatValue, float maxFloatValue, float floatStep,
        const SliderStyle& sliderStyle, std::string FloatSliderTextItself)
    {
        if (!panelOpen) return false;

        Color FloatTrackColor                  = sliderStyle.trackColor;
        Color FloatHandleColor                 = sliderStyle.handleColor;
        Color FloatSliderTextColor             = sliderStyle.textColor;
        float FloatSliderTextSize              = sliderStyle.textSize;
        std::string FloatSliderTrackImagePath  = sliderStyle.trackImagePath;
        std::string FloatSliderHandleImagePath = sliderStyle.handleImagePath;
        std::string FloatSliderFontPath        = sliderStyle.fontPath;

        bool changed = false;
        float handleWidth = FloatSliderRect.height;

        // ===[Hover must be known before we ask whether this widget owns the drag]===
        bool hoveredFloatTrack = WidgetIsHovered(&value, FloatSliderRect);
        if (hoveredFloatTrack) mouseHoveredOverWidgetThisFrame = true;

        // ===[Apply a drag directly as a continuous value, no stepping needed]===
        float normalizedDragPositionThisFrame = SliderNormalizedDragValue(&value, FloatSliderRect, handleWidth, hoveredFloatTrack);
        if (normalizedDragPositionThisFrame >= 0.0f)
        {
            float draggedValue = minFloatValue + normalizedDragPositionThisFrame * (maxFloatValue - minFloatValue);
            if (draggedValue != value)
            {
                value = draggedValue;
                changed = true;
            }
        }

        // ===[Keyboard nudging while hovered]===
        if (hoveredFloatTrack)
        {
            float keyboardNudgeStep = (maxFloatValue - minFloatValue) * 0.01f; // 1% per keypress
            if (leftArrowKeyPressed || downArrowKeyPressed)
            {
                float nudgedValue = std::clamp(value - keyboardNudgeStep, minFloatValue, maxFloatValue);
                if (nudgedValue != value) { value = nudgedValue; changed = true; }
            }
            if (rightArrowKeyPressed || upArrowKeyPressed)
            {
                float nudgedValue = std::clamp(value + keyboardNudgeStep, minFloatValue, maxFloatValue);
                if (nudgedValue != value) { value = nudgedValue; changed = true; }
            }
        }

        // ===[Handle position always tracks the actual current value, not the transient drag delta,
        //     so it stays correct on frames where the slider isn't being actively dragged]===
        float currentNormalizedPosition = (maxFloatValue != minFloatValue)
            ? (value - minFloatValue) / (maxFloatValue - minFloatValue)
            : 0.0f;
        float handleX = FloatSliderRect.xPos + currentNormalizedPosition * (FloatSliderRect.width - handleWidth);
        Rect handleRect{ handleX, FloatSliderRect.yPos, handleWidth, FloatSliderRect.height, 0.0f };

        // ===[Handle darkening: same has-image vs no-image split as Button]===
        bool isActiveFloatSlider = (currentFrameActiveWidgetIdentifier == &value);
        float handleDarkenAmount = 1.0f;
        if (isActiveFloatSlider)     handleDarkenAmount = kActiveDarkenFactor;
        else if (hoveredFloatTrack)  handleDarkenAmount = kHoverDarkenFactor;

        // ===[Draw optional texture, and both the Slider Handle and Track]===
        GLTexture* floatTrackTexture  = FloatSliderTrackImagePath.empty()  ? nullptr : GetOrLoadTexture(FloatSliderTrackImagePath);
        GLTexture* floatHandleTexture = FloatSliderHandleImagePath.empty() ? nullptr : GetOrLoadTexture(FloatSliderHandleImagePath);
        DrawQuad(FloatSliderRect, FloatTrackColor, floatTrackTexture);
        // No image -> pre-darken the flat handle color. Has image -> pass the darken
        // factor through and let the shader darken the texture instead.
        Color finalHandleColor = floatHandleTexture ? FloatHandleColor : ApplyDarken(FloatHandleColor, handleDarkenAmount);
        DrawQuad(handleRect, finalHandleColor, floatHandleTexture, handleDarkenAmount);

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
    bool Switch(Rect SwitchRect, bool& value, const SwitchStyle& switchStyle,
        std::string SwitchOnTextItself, std::string SwitchOffTextItself)
    {
        if (!panelOpen) return false;

        Color SwitchOnColor             = switchStyle.onColor;
        Color SwitchOffColor            = switchStyle.offColor;
        Color SwitchTextColor           = switchStyle.textColor;
        float SwitchTextSize            = switchStyle.textSize;
        std::string SwitchOnImagePath   = switchStyle.onImagePath;
        std::string SwitchOffImagePath  = switchStyle.offImagePath;
        std::string SwitchFontPath      = switchStyle.fontPath;

        bool hoveredSwitch = WidgetIsHovered(&value, SwitchRect);
        if (hoveredSwitch) mouseHoveredOverWidgetThisFrame = true;

        // ===[A click anywhere on the switch flips its boolean state]===
        bool toggled = false;
        if (hoveredSwitch && leftMouseButtonPressed)
        {
            value = !value;
            toggled = true;
        }

        // ===[Handle darkening: same has-image vs no-image split as Button]===
        bool pressedSwitch = hoveredSwitch && leftMouseButtonDown;
        float switchDarkenAmount = 1.0f;
        if (pressedSwitch)        switchDarkenAmount = kActiveDarkenFactor;
        else if (hoveredSwitch)   switchDarkenAmount = kHoverDarkenFactor;

        const std::string& activeImagePath = value ? SwitchOnImagePath : SwitchOffImagePath;
        GLTexture* switchTexture = activeImagePath.empty() ? nullptr : GetOrLoadTexture(activeImagePath);
        Color baseSwitchColor = value ? SwitchOnColor : SwitchOffColor;
        Color finalSwitchColor = switchTexture ? baseSwitchColor : ApplyDarken(baseSwitchColor, switchDarkenAmount);
        DrawQuad(SwitchRect, finalSwitchColor, switchTexture, switchDarkenAmount);

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
    void Image(Rect ImageRect, const ImageStyle& imageStyle)
    {
        if (!panelOpen) return;

        GLTexture* imageTexture = GetOrLoadTexture(imageStyle.imagePath);
        DrawQuad(ImageRect, imageStyle.color, imageTexture);
    }
    // void Separator(Rect SeparatorRect, Color SeparatorColor)
    // {
    //     if (!panelOpen) return;

    //     DrawQuad(SeparatorRect, SeparatorColor, nullptr);
    // }
    void Text(Rect TextRect, const TextStyle& textStyle, std::string TextItself)
    {
        if (!panelOpen) return;

        Font* font = GetOrLoadFont(textStyle.fontPath, (unsigned int)textStyle.size);
        if (!font) return;

        float bottomUpY = SCREEN_HEIGHT - TextRect.yPos;
        float angleRadians = glm::radians(TextRect.rotation);

        DrawTextRaw(TextRect.xPos, bottomUpY, textStyle.color, textStyle.size, TextItself, font, angleRadians);
    }
    bool MultiInputField(Rect inputFieldRect, std::string& textValue, const InputFieldStyle& inputFieldStyle)
    {
        if (!panelOpen) return false;

        bool fieldIsFocused = BeginInputField(&textValue, inputFieldRect, inputFieldStyle.backGroundColor, inputFieldStyle.imagePath);
        size_t& caretIndex = caretIndexMap[&textValue];

        bool textChanged = false;
        if (fieldIsFocused) textChanged = ApplyTypingToText(textValue, caretIndex, true, "");

        DrawInputFieldText(inputFieldRect, textValue, caretIndex, fieldIsFocused, inputFieldStyle.textColor,
            inputFieldStyle.textSize, inputFieldStyle.fontPath, nullptr);
        return textChanged;
    }
    bool NumberInputField(Rect inputFieldRect, float& numberValue, const InputFieldStyle& inputFieldStyle)
    {
        if (!panelOpen) return false;

        bool fieldWasFocusedBefore = (focusedInputFieldIdentifier == &numberValue);
        bool fieldIsFocused = BeginInputField(&numberValue, inputFieldRect, inputFieldStyle.backGroundColor, inputFieldStyle.imagePath);
        size_t& caretIndex = caretIndexMap[&numberValue];

        // Turn the current number into editable text when the field first gains focus
        std::string& editBuffer = numberFieldEditBufferMap[&numberValue];
        if (fieldIsFocused && !fieldWasFocusedBefore)
        {
            char formattedNumber[32];
            std::snprintf(formattedNumber, sizeof(formattedNumber), "%g", numberValue);
            editBuffer = formattedNumber;
        }

        // While focused, edit the buffer and push every valid parse into the real value
        bool numberChanged = false;
        if (fieldIsFocused && ApplyTypingToText(editBuffer, caretIndex, false, "0123456789.-"))
        {
            char* parseEndPointer = nullptr;
            float parsedNumber = std::strtof(editBuffer.c_str(), &parseEndPointer);
            bool bufferIsValidNumber = !editBuffer.empty() && parseEndPointer != editBuffer.c_str();
            if (bufferIsValidNumber && parsedNumber != numberValue)
            {
                numberValue = parsedNumber;
                numberChanged = true;
            }
        }

        // Show the raw buffer while editing, otherwise the formatted value
        std::string textToShow = editBuffer;
        if (!fieldIsFocused)
        {
            char formattedNumber[32];
            std::snprintf(formattedNumber, sizeof(formattedNumber), "%g", numberValue);
            textToShow = formattedNumber;
        }

        DrawInputFieldText(inputFieldRect, textToShow, caretIndex, fieldIsFocused, inputFieldStyle.textColor,
            inputFieldStyle.textSize, inputFieldStyle.fontPath, nullptr);
        return numberChanged;
    }
    bool CodeInputField(Rect inputFieldRect, std::string& textValue, const CodeInputFieldStyle& codeInputFieldStyle)
    {
        if (!panelOpen) return false;

        bool fieldIsFocused = BeginInputField(&textValue, inputFieldRect, codeInputFieldStyle.backGroundColor, codeInputFieldStyle.imagePath);
        size_t& caretIndex = caretIndexMap[&textValue];

        bool textChanged = false;
        if (fieldIsFocused) textChanged = ApplyTypingToText(textValue, caretIndex, true, "");

        DrawInputFieldText(inputFieldRect, textValue, caretIndex, fieldIsFocused, codeInputFieldStyle.textColor,
            codeInputFieldStyle.textSize, codeInputFieldStyle.fontPath, &codeInputFieldStyle);
        return textChanged;
    }

    // |=====================================================
    // |---[ALL OUR FOOD KEEPS BLOWING UP]-------------------
    // |=====================================================
    void BlowUp()
    {
        // ===[Free every cached texture]===
        for (auto& [path, tex] : textureCache)
        {
            if (!tex) continue;
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
