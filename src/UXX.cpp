#include "UXX.hpp"

namespace UXX
{
    // |=====================================================
    // |---[Private state]-----------------------------------
    // |=====================================================
    namespace
    {
        // ===[Shader stuff]===
        VAO* vao        = nullptr;
        VBO* vbo        = nullptr;
        EBO* ebo        = nullptr;
        Shader* shader  = nullptr;

        // ===[Shader uniform locations]===
        GLint colorUniformLocation      = -1;
        GLint sizeUniformLocation       = -1;
        GLint positionUniformLocation   = -1;
        GLint rotationUniformLocation   = -1;
        GLint useTextureUniformLocation = -1;
        GLint darkenUniformLocation     = -1;

        GLint textColorUniformLocation  = -1;
        GLint textModelUniformLocation  = -1;

        // ===[Caches]===
        std::unordered_map<std::string, GLTexture*> textureCache;
        GLTexture* GetOrLoadTexture(const std::string& path);

        std::unordered_map<std::string, Font*> fontCache;
        Font* GetOrLoadFont(const std::string& path, unsigned int pixelHeight = 48);

        // Forward declaration needed because SliderNormalizedDragValue calls WidgetClaimsActive
        // before its own definition appears in this file
        bool WidgetClaimsActive(const void* widgetIdentifier, bool widgetIsHoveredThisFrame);

        // ===[Wiget variables]===
        bool panelOpen              = false;
        bool scrollBoxOpen          = false;
        bool dropDownOpen           = false;

        float scrollBarWidth              = 16.0f;  // Width of the scrollbar track
        float scrollBarMinimumHandleSize  = 20.0f;  // Handle never gets smaller than this
        float mouseWheelScrollPixels      = 40.0f;  // Pixels scrolled per wheel notch

        // Which drop down is currently expanded, keyed by the address of its selectedIndex
        const void* expandedDropDownIdentifier = nullptr;
        // Each drop down remembers its own list scroll position
        std::unordered_map<const void*, float> dropDownScrollOffsetMap;

        // ===[Keyboard]===
        bool leftArrowKeyPressed    = false;
        bool rightArrowKeyPressed   = false;
        bool upArrowKeyPressed      = false;
        bool downArrowKeyPressed    = false;

        // ===[Shared hover/active widget ownership, used by every widget type]===
        const void* currentFrameHoveredWidgetIdentifier = nullptr;
        const void* currentFrameActiveWidgetIdentifier = nullptr;

        // ===[Mouse]===
        bool leftMouseButtonDown                = false;
        bool leftMouseButtonPressed             = false;
        bool leftMouseButtonReleased            = false;

        bool rightMouseButtonDown               = false;
        bool rightMouseButtonPressed            = false;
        bool rightMouseButtonReleased           = false;

        bool middleMouseButtonDown              = false;
        bool middleMouseButtonPressed           = false;
        bool middleMouseButtonReleased          = false;
        bool mouseHoveredOverWidgetThisFrame    = false;

        double mousePositionX       = -1.0, mousePositionY = -1.0;
        double scrollDeltaXState    = 0.0;
        double scrollDeltaYState    = 0.0;

        // ===[Adding Input clipping to widgets that are inside the scroll box and panel rect]===
        bool inputClipIsActive = false;
        Rect inputClipRect(0, 0, 0, 0);
        // True when no clip is active, or the cursor is inside the visible clip area
        bool CursorIsInsideInputClip()
        {
            if (!inputClipIsActive) return true;

            return mousePositionX >= inputClipRect.xPos && mousePositionX <= inputClipRect.xPos + inputClipRect.width &&
                   mousePositionY >= inputClipRect.yPos && mousePositionY <= inputClipRect.yPos + inputClipRect.height;
        }

        // ===[Square like Minecraft]===
        // Shared unit-quad geometry reused by every UI element
        float rectangleVertices[] =
        {
            //  Positions           Color               Texture Coords
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
        // |=====================================================
        // |---[Getting or Loading]------------------------------
        // |=====================================================
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
        constexpr float hoverDarkenFactor  = 0.85f;
        constexpr float activeDarkenFactor = 0.70f;

        // |=====================================================
        // |---[Drawing]-----------------------------------------
        // |=====================================================
        // ===[Be there or be SQUARE]===
        void DrawQuad(Rect rect, Color color, GLTexture* texture, float darkenAmount = 1.0f)
        {
            shader->Use();

            // Color of Quad
            glUniform4f(colorUniformLocation, color.red, color.green, color.blue, color.alpha);
            // Size of Quad
            glUniform2f(sizeUniformLocation, (rect.width / SCREEN_WIDTH) * 2.0f, (rect.height / SCREEN_HEIGHT) * 2.0f);
            // Position of Quad
            glUniform2f(positionUniformLocation,
                ((rect.xPos + rect.width * 0.5f) / SCREEN_WIDTH) * 2.0f - 1.0f,
                1.0f - ((rect.yPos + rect.height * 0.5f) / SCREEN_HEIGHT) * 2.0f);
            // Rotation of Quad
            glUniform1f(rotationUniformLocation, rect.rotation);
            // Tell the shader whether to sample tex0 or just use the flat color
            glUniform1i(textColorUniformLocation, texture ? 1 : 0);
            // Darken of Quad when hoverd/clicked
            glUniform1f(darkenUniformLocation, darkenAmount);

            if (texture) texture->Bind();
            vao->Bind();
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            vao->Unbind();
            if (texture) texture->Unbind();
        }
        // ===[Don't Draw Quad >:) ]===
        bool SkipDraw(Rect rectToSkip)
        {
            // Skip any widget if their width and position are outside the users screen
            if (rectToSkip.xPos + rectToSkip.width > SCREEN_WIDTH ||
                rectToSkip.yPos + rectToSkip.height > SCREEN_HEIGHT) return false;

            return false;
        }
        // ===[Shared shader/uniform setup + draw call for any piece of text]===
        void DrawTextRaw(float textPositionX, float bottomUpY, Color color, float size, const std::string& text, Font* font, float angleRadians)
        {
            if (!font) return;

            textShader->Use();

            // Set up an orthographic projection matching the current screen size
            glm::mat4 projection = glm::ortho(0.0f, SCREEN_WIDTH, 0.0f, SCREEN_HEIGHT);
            textShader->setMat4("projection", projection);
            glUniform4f(textColorUniformLocation, color.red, color.green, color.blue, color.alpha);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            font->rendererText(*textShader, text, textPositionX, bottomUpY, size, textVAO, textVBO, textModelUniformLocation, angleRadians);
        }

        // |=====================================================
        // |---[Widget Functionality]----------------------------
        // |=====================================================
        // ===[Shared drag math for both Int and Float sliders]===
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
            if (cursorInsideBounds && CursorIsInsideInputClip())
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

        // |=====================================================
        // |---[Cut out anything that are outside somthing]------
        // |=====================================================
        // Stack of scissor boxes so nested scissors restore properly
        std::vector<std::array<GLint,4>> scissorStack;
        std::vector<GLboolean> scissorEnabledStack;
        // ===[Push a scissor rect, intersected with whatever scissor is already active so nested clips only ever shrink]===
        void PushScissor(Rect rect)
        {
            GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
            GLint previousScissorBox[4];
            glGetIntegerv(GL_SCISSOR_BOX, previousScissorBox);

            scissorEnabledStack.push_back(scissorWasEnabled);
            scissorStack.push_back({ previousScissorBox[0], previousScissorBox[1], previousScissorBox[2], previousScissorBox[3] });

            GLint newScissorLeft   = (GLint)(rect.xPos * SCREEN_SCALE_X);
            GLint newScissorBottom = (GLint)((SCREEN_HEIGHT - (rect.yPos + rect.height)) * SCREEN_SCALE_Y);
            GLint newScissorRight  = newScissorLeft + (GLint)(rect.width  * SCREEN_SCALE_X);
            GLint newScissorTop    = newScissorBottom + (GLint)(rect.height * SCREEN_SCALE_Y);

            // Only intersect when a scissor was really active, otherwise the box is just the default full window
            if (scissorWasEnabled)
            {
                GLint previousScissorRight = previousScissorBox[0] + previousScissorBox[2];
                GLint previousScissorTop   = previousScissorBox[1] + previousScissorBox[3];

                newScissorLeft   = std::max(newScissorLeft,   previousScissorBox[0]);
                newScissorBottom = std::max(newScissorBottom, previousScissorBox[1]);
                newScissorRight  = std::min(newScissorRight,  previousScissorRight);
                newScissorTop    = std::min(newScissorTop,    previousScissorTop);
            }

            // An empty intersection becomes a zero-size scissor, which clips everything
            GLsizei intersectedScissorWidth  = (GLsizei)std::max(0, newScissorRight - newScissorLeft);
            GLsizei intersectedScissorHeight = (GLsizei)std::max(0, newScissorTop - newScissorBottom);

            glEnable(GL_SCISSOR_TEST);
            glScissor(newScissorLeft, newScissorBottom, intersectedScissorWidth, intersectedScissorHeight);
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

        // |=====================================================
        // |---[Input Field Functionality]-----------------------
        // |=====================================================

        // ===[Per-frame input, filled by the backend]===
        // Characters typed this frame, in the order they were typed
        std::string typedCharactersThisFrame;

        // ===[How many times each editing key was pressed this frame]===
        int backspacePressCountThisFrame    = 0;
        int enterPressCountThisFrame        = 0;
        int leftArrowPressCountThisFrame    = 0;
        int rightArrowPressCountThisFrame   = 0;
        int upArrowPressCountThisFrame      = 0;
        int downArrowPressCountThisFrame    = 0;

        int homePressCountThisFrame         = 0;
        int endPressCountThisFrame          = 0;
        int deletePressCountThisFrame       = 0;

        // ===[Shortcut input, filled by the backend every frame]===
        bool copyShortcutPressedSinceLastPoll       = false;
        bool cutShortcutPressedSinceLastPoll        = false;
        bool pasteShortcutPressedSinceLastPoll      = false;

        bool tabShortcutPressedSinceLastPoll        = false;
        bool shiftTabShortcutPressedSinceLastPoll   = false;

        // Text from the clipboard (for paste) and text waiting to be sent to it (for copy/cut)
        std::string clipboardTextThisFrame;
        std::string textWaitingToBeCopied;

        // How many spaces one Tab press inserts
        constexpr int tabSpaceCount = 4;

        // ===[Per-field state, remembered between frames]===
        // Which input field currently receives typing
        const void* focusedInputFieldIdentifier = nullptr;
        // Caret position inside each field, keyed by the address of the field's value
        std::unordered_map<const void*, size_t> caretIndexMap;
        // Number fields edit a text copy while focused ("3." or "-" is not a valid float yet)
        std::unordered_map<const void*, std::string> numberFieldEditBufferMap;

        // ===[Constants]===
        // An empty allowed list means every character is accepted
        const std::string anyCharacterAllowed = "";
        const std::string numberFieldAllowedCharacters = "0123456789.-/";
        const std::string bracketCharacters = "()[]{}<>";

        constexpr float inputFieldPadding = 8.0f;                   // Space between field edge and text, in pixels
        constexpr float inputFieldLineHeightMultiplier = 1.5f;      // Line height as a multiple of glyph height
        constexpr float caretWidth = 2.0f;                          // In pixels
        constexpr double caretBlinkHalfPeriodSeconds = 0.5;         // Time the caret stays on, then off

        // |=====================================================
        // |---[Small text helpers]------------------------------
        // |=====================================================
        // ===[Turns a float into clean text, so 3.5000 shows as 3.5 and 2.0000 as 2]===
        std::string FormatNumberAsText(float numberToFormat)
        {
            char formattedNumberBuffer[64];
            std::snprintf(formattedNumberBuffer, sizeof(formattedNumberBuffer), "%.4f", numberToFormat);
            std::string formattedNumberText = formattedNumberBuffer;

            // Only trim zeros when there is a decimal point, otherwise "100" would lose its zeros
            if (formattedNumberText.find('.') != std::string::npos)
            {
                formattedNumberText.erase(formattedNumberText.find_last_not_of('0') + 1);
                if (formattedNumberText.back() == '.') formattedNumberText.pop_back();
            }
            return formattedNumberText;
        }
        bool IsBracketCharacter(char character)
        {
            return bracketCharacters.find(character) != std::string::npos;
        }

        // |=====================================================
        // |---[Focus]-------------------------------------------
        // |=====================================================
        // ===[Click inside to focus, click outside to unfocus. Returns true if this field is focused]===
        bool UpdateInputFieldFocus(const void* widgetIdentifier, bool widgetIsHoveredThisFrame)
        {
            // Focus only changes on the frame the left mouse button goes down
            if (!leftMouseButtonPressed) return focusedInputFieldIdentifier == widgetIdentifier;

            if (widgetIsHoveredThisFrame)
            {
                // Newly focused fields start with the caret at the end (npos is clamped to the text size later)
                if (focusedInputFieldIdentifier != widgetIdentifier)
                    caretIndexMap[widgetIdentifier] = std::string::npos;

                focusedInputFieldIdentifier = widgetIdentifier;
            }
            else if (focusedInputFieldIdentifier == widgetIdentifier)
            {
                // Clicked somewhere else while this field was focused
                focusedInputFieldIdentifier = nullptr;
            }

            return focusedInputFieldIdentifier == widgetIdentifier;
        }

        // |=====================================================
        // |---[Lines and caret movement]------------------------
        // |=====================================================
        // The caret index is a position in the WHOLE text, so these helpers find where lines begin and end.

        // ===[Index of the first character on the line that holds the caret]===
        size_t FindLineStartIndex(const std::string& textToSearch, size_t caretIndex)
        {
            if (caretIndex == 0) return 0;

            // Search backwards from just before the caret so a caret sitting on a newline stays on its own line
            size_t previousNewlineIndex = textToSearch.rfind('\n', caretIndex - 1);
            return previousNewlineIndex == std::string::npos ? 0 : previousNewlineIndex + 1;
        }
        // ===[Index of the newline ending the caret's line, or the text size on the last line]===
        size_t FindLineEndIndex(const std::string& textToSearch, size_t caretIndex)
        {
            size_t nextNewlineIndex = textToSearch.find('\n', caretIndex);
            return nextNewlineIndex == std::string::npos ? textToSearch.size() : nextNewlineIndex;
        }
        // ===[Moves the caret one line up (-1) or down (+1), keeping the same column when the target line is long enough]===
        void MoveCaretToNeighborLine(const std::string& textToSearch, size_t& caretIndex, int lineDirection)
        {
            size_t currentLineStartIndex = FindLineStartIndex(textToSearch, caretIndex);
            size_t currentLineEndIndex = FindLineEndIndex(textToSearch, caretIndex);
            size_t columnIndex = caretIndex - currentLineStartIndex;

            size_t targetLineStartIndex = 0;
            size_t targetLineEndIndex = 0;

            if (lineDirection < 0)
            {
                // Already on the first line, nowhere to go
                if (currentLineStartIndex == 0) return;

                // The newline just before this line is the end of the line above
                targetLineEndIndex = currentLineStartIndex - 1;
                targetLineStartIndex = FindLineStartIndex(textToSearch, targetLineEndIndex);
            }
            else
            {
                // Already on the last line, nowhere to go
                if (currentLineEndIndex >= textToSearch.size()) return;

                // The line below starts right after the newline that ends this one
                targetLineStartIndex = currentLineEndIndex + 1;
                targetLineEndIndex = FindLineEndIndex(textToSearch, targetLineStartIndex);
            }

            // A shorter target line clamps the caret to its end
            size_t targetLineLength = targetLineEndIndex - targetLineStartIndex;
            caretIndex = targetLineStartIndex + std::min(columnIndex, targetLineLength);
        }

        // |=====================================================
        // |---[Applying this frame's keys to the text]----------
        // |=====================================================
        // Each step below handles ONE kind of key. ApplyTypingToText just runs them in order.

        // ===[Arrow keys: left/right always work, up/down only in multi-line fields]===
        void MoveCaretWithArrowKeys(const std::string& textToEdit, size_t& caretIndex, bool allowNewlines)
        {
            for (int pressIndex = 0; pressIndex < leftArrowPressCountThisFrame; pressIndex++)
                if (caretIndex > 0) caretIndex--;

            for (int pressIndex = 0; pressIndex < rightArrowPressCountThisFrame; pressIndex++)
                if (caretIndex < textToEdit.size()) caretIndex++;

            if (!allowNewlines) return;

            for (int pressIndex = 0; pressIndex < upArrowPressCountThisFrame; pressIndex++)
                MoveCaretToNeighborLine(textToEdit, caretIndex, -1);
            for (int pressIndex = 0; pressIndex < downArrowPressCountThisFrame; pressIndex++)
                MoveCaretToNeighborLine(textToEdit, caretIndex, +1);
        }
        // ===[Home and End jump to the start or end of the current line]===
        void MoveCaretWithHomeAndEndKeys(const std::string& textToEdit, size_t& caretIndex)
        {
            if (homePressCountThisFrame > 0) caretIndex = FindLineStartIndex(textToEdit, caretIndex);
            if (endPressCountThisFrame > 0)  caretIndex = FindLineEndIndex(textToEdit, caretIndex);
        }
        // ===[Inserts typed characters at the caret, skipping ones the field does not allow]===
        bool InsertTypedCharacters(std::string& textToEdit, size_t& caretIndex, const std::string& allowedCharacters)
        {
            bool textChanged = false;

            for (char typedCharacter : typedCharactersThisFrame)
            {
                bool characterIsAllowed = allowedCharacters.empty() ||
                                          allowedCharacters.find(typedCharacter) != std::string::npos;
                if (!characterIsAllowed) continue;

                textToEdit.insert(caretIndex, 1, typedCharacter);
                caretIndex++;
                textChanged = true;
            }
            return textChanged;
        }
        // ===[Backspace removes the character before the caret, Delete removes the one after it]===
        bool DeleteCharactersAroundCaret(std::string& textToEdit, size_t& caretIndex)
        {
            bool textChanged = false;

            for (int pressIndex = 0; pressIndex < backspacePressCountThisFrame && caretIndex > 0; pressIndex++)
            {
                textToEdit.erase(caretIndex - 1, 1);
                caretIndex--;
                textChanged = true;
            }

            // Delete leaves the caret where it is
            for (int pressIndex = 0; pressIndex < deletePressCountThisFrame && caretIndex < textToEdit.size(); pressIndex++)
            {
                textToEdit.erase(caretIndex, 1);
                textChanged = true;
            }
            return textChanged;
        }
        // ===[Enter adds a new line in multi-line fields, otherwise it ends editing]===
        bool HandleEnterKey(std::string& textToEdit, size_t& caretIndex, bool allowNewlines)
        {
            bool textChanged = false;

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
        // ===[Applies this frame's caret movement and typing, returns true if the text changed]===
        bool ApplyTypingToText(std::string& textToEdit, size_t& caretIndex, bool allowNewlines, const std::string& allowedCharacters)
        {
            // Turns the "end of text" marker (npos) into a real index
            caretIndex = std::min(caretIndex, textToEdit.size());

            MoveCaretWithArrowKeys(textToEdit, caretIndex, allowNewlines);
            MoveCaretWithHomeAndEndKeys(textToEdit, caretIndex);

            bool textChanged = false;
            textChanged |= InsertTypedCharacters(textToEdit, caretIndex, allowedCharacters);
            textChanged |= DeleteCharactersAroundCaret(textToEdit, caretIndex);
            textChanged |= HandleEnterKey(textToEdit, caretIndex, allowNewlines);
            return textChanged;
        }

        // |=====================================================
        // |---[Drawing the field text]--------------------------
        // |=====================================================
        // ===[Picks the bracket color for a character, or the normal text color]===
        Color PickCodeCharacterColor(char character, const CodeInputFieldStyle& codeStyle)
        {
            if (character == '(' || character == ')') return codeStyle.roundBracketColor;
            if (character == '[' || character == ']') return codeStyle.squareBracketColor;
            if (character == '{' || character == '}') return codeStyle.curlyBracketColor;
            if (character == '<' || character == '>') return codeStyle.angleBracketColor;
            return codeStyle.textColor;
        }
        // ===[Draws one line of code. Normal text is one run until the next bracket, every bracket is its own run]===
        void DrawCodeLineWithColoredBrackets(float lineStartX, float baselineBottomUpY, const std::string& lineText,
            float textSize, Font* font, const CodeInputFieldStyle& codeStyle)
        {
            float currentRunStartX = lineStartX;
            size_t runStartIndex = 0;

            while (runStartIndex < lineText.size())
            {
                size_t runEndIndex = runStartIndex + 1;

                // Normal text keeps growing until the next bracket
                if (!IsBracketCharacter(lineText[runStartIndex]))
                    while (runEndIndex < lineText.size() && !IsBracketCharacter(lineText[runEndIndex]))
                        runEndIndex++;

                std::string runText = lineText.substr(runStartIndex, runEndIndex - runStartIndex);
                DrawTextRaw(currentRunStartX, baselineBottomUpY,
                            PickCodeCharacterColor(lineText[runStartIndex], codeStyle),
                            textSize, runText, font, 0.0f);

                // The next run starts where this one ended
                currentRunStartX += font->measureTextWidth(runText, textSize);
                runStartIndex = runEndIndex;
            }
        }
        // ===[Draws the blinking caret at its real position]===
        void DrawCaret(Rect fieldRect, const std::string& textToDraw, size_t caretIndex,
            float lineHeight, Color caretColor, float textSize, Font* font)
        {
            bool caretIsInOnPhase = std::fmod(glfwGetTime(), caretBlinkHalfPeriodSeconds * 2.0) < caretBlinkHalfPeriodSeconds;
            if (!caretIsInOnPhase) return;

            // Which line the caret is on = how many newlines come before it
            int caretLineNumber = (int)std::count(textToDraw.begin(), textToDraw.begin() + caretIndex, '\n');

            // Horizontal position = width of the text between the line start and the caret
            size_t caretLineStartIndex = FindLineStartIndex(textToDraw, caretIndex);
            std::string textBeforeCaretOnItsLine = textToDraw.substr(caretLineStartIndex, caretIndex - caretLineStartIndex);

            float caretX = fieldRect.xPos + inputFieldPadding + font->measureTextWidth(textBeforeCaretOnItsLine, textSize);
            float caretTopY = fieldRect.yPos + inputFieldPadding + lineHeight * caretLineNumber;
            DrawQuad(Rect(caretX, caretTopY, caretWidth, lineHeight), caretColor, nullptr);
        }
        // ===[Draws all text line by line plus a blinking caret, brackets are colored if codeStyle is given]===
        void DrawInputFieldText(Rect fieldRect, const std::string& textToDraw, size_t caretIndex, bool fieldIsFocused,
            Color textColor, float textSize, const std::string& fontPath,
            const CodeInputFieldStyle* codeStyle)
        {
            Font* font = GetOrLoadFont(fontPath, (unsigned int)textSize);
            if (!font) return;

            // Anything that does not fit inside the field gets cut off
            PushScissor(fieldRect);

            float lineHeight = font->measureTextHeight("A", textSize) * inputFieldLineHeightMultiplier;
            caretIndex = std::min(caretIndex, textToDraw.size());
            float lineStartX = fieldRect.xPos + inputFieldPadding;

            // Walk through the text one line at a time
            size_t lineStartIndex = 0;
            int lineNumber = 0;
            while (lineStartIndex <= textToDraw.size())
            {
                size_t lineEndIndex = FindLineEndIndex(textToDraw, lineStartIndex);
                std::string lineText = textToDraw.substr(lineStartIndex, lineEndIndex - lineStartIndex);

                // Text is drawn from a baseline measured from the bottom of the screen
                float baselineTopDownY = fieldRect.yPos + inputFieldPadding + lineHeight * (lineNumber + 1);
                float baselineBottomUpY = SCREEN_HEIGHT - baselineTopDownY;

                // Code fields color the brackets, other fields draw the whole line in one color
                if (codeStyle)
                    DrawCodeLineWithColoredBrackets(lineStartX, baselineBottomUpY, lineText, textSize, font, *codeStyle);
                else
                    DrawTextRaw(lineStartX, baselineBottomUpY, textColor, textSize, lineText, font, 0.0f);

                // Next line starts after the newline (this goes past the text size after the last line, which ends the loop)
                lineStartIndex = lineEndIndex + 1;
                lineNumber++;
            }

            if (fieldIsFocused)
                DrawCaret(fieldRect, textToDraw, caretIndex, lineHeight, textColor, textSize, font);

            PopScissor();
        }

        // ===[Shared start of every input field: hover, focus and the background]===
        // Returns true when this field is the one that currently receives typing.
        bool BeginInputField(const void* widgetIdentifier, Rect fieldRect, Color backgroundColor,
                             const std::string& imagePath)
        {
            // Needed so the cursor turns into a hand over the field
            bool fieldIsHovered = WidgetIsHovered(widgetIdentifier, fieldRect);
            if (fieldIsHovered) mouseHoveredOverWidgetThisFrame = true;

            bool fieldIsFocused = UpdateInputFieldFocus(widgetIdentifier, fieldIsHovered);

            // Optional image background, otherwise a flat color
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

        leftMouseButtonDown         = leftDown;
        leftMouseButtonPressed      = leftPressed;
        leftMouseButtonReleased     = leftReleased;
        rightMouseButtonDown        = rightDown;
        rightMouseButtonPressed     = rightPressed;
        rightMouseButtonReleased    = rightReleased;
        middleMouseButtonDown       = middleDown;
        middleMouseButtonPressed    = middlePressed;
        middleMouseButtonReleased   = middleReleased;

        scrollDeltaXState = scrollDeltaX;
        scrollDeltaYState = scrollDeltaY;

        mouseHoveredOverWidgetThisFrame     = false;
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
        leftArrowKeyPressed     = leftArrowPressed;
        rightArrowKeyPressed    = rightArrowPressed;
        upArrowKeyPressed       = upArrowPressed;
        downArrowKeyPressed     = downArrowPressed;
    }
    bool MouseHoveredOverWidget()
    {
        return mouseHoveredOverWidgetThisFrame;
    }
    void SetTextInputState(const std::string& typedCharacters, int backspacePressCount, int enterPressCount)
    {
        typedCharactersThisFrame        = typedCharacters;
        backspacePressCountThisFrame    = backspacePressCount;
        enterPressCountThisFrame        = enterPressCount;
    }
    // Receives how many times each editing key was pressed this frame
    void SetEditKeyState(int leftArrowPressCount, int rightArrowPressCount, int upArrowPressCount,
        int downArrowPressCount, int homePressCount, int endPressCount, int deletePressCount)
    {
        leftArrowPressCountThisFrame    = leftArrowPressCount;
        rightArrowPressCountThisFrame   = rightArrowPressCount;
        upArrowPressCountThisFrame      = upArrowPressCount;
        downArrowPressCountThisFrame    = downArrowPressCount;

        homePressCountThisFrame         = homePressCount;
        endPressCountThisFrame          = endPressCount;
        deletePressCountThisFrame       = deletePressCount;
    }
    void SetShortCutKeyState(bool copyShortcutPressed, bool cutShortcutPressed, bool pasteShortcutPressed)
    {

    }
    void SetCodeShortCutKeyState(bool tabShortcutPressed, bool shiftTabShortcutPressed)
    {

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
        colorUniformLocation        = glGetUniformLocation(shader->ID, "uColor");
        sizeUniformLocation         = glGetUniformLocation(shader->ID, "uSize");
        positionUniformLocation     = glGetUniformLocation(shader->ID, "uPosition");
        rotationUniformLocation     = glGetUniformLocation(shader->ID, "uRotation");
        textColorUniformLocation    = glGetUniformLocation(shader->ID, "uUseTexture");
        darkenUniformLocation       = glGetUniformLocation(shader->ID, "uDarken");

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        InitTextRendering();
    }
    void InitTextRendering()
    {
        // Separate shader and VAO/VBO pair dedicated to glyph quads
        textShader                  = new Shader("../shader_files/VertexText.glsl", "../shader_files/FragmentText.glsl");
        textColorUniformLocation    = glGetUniformLocation(textShader->ID, "textColor");
        textModelUniformLocation    = glGetUniformLocation(textShader->ID, "model");

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
    void BeginPanel(Rect panelRect, Color panelColor, std::string panelImagePath)
    {
        panelOpen = true;
        SkipDraw(panelRect);

        // Draw the panel background, optionally textured
        GLTexture* panelTexture = panelImagePath.empty() ? nullptr : GetOrLoadTexture(panelImagePath);
        DrawQuad(panelRect, panelColor, panelTexture);

        PushScissor(panelRect);

        // ===[Clip drawing and mouse input to the content area, so nothing covers or reacts through the panel]===
        Rect panelContentRect(panelRect.xPos, panelRect.yPos, panelRect.width, panelRect.height);
        PushScissor(panelContentRect);

        inputClipRect = panelContentRect;
        inputClipIsActive = true;
    }
    void EndPanel()
    {
        panelOpen = false;
        inputClipIsActive = false;
        PopScissor();
    }

    // |=====================================================
    // |---[Common UI stuff]---------------------------------
    // |=====================================================
    bool Button(Rect ButtonRect, const ButtonStyle& buttonStyle, std::string ButtonTextItself)
    {
        if (!panelOpen) return false;
        SkipDraw(ButtonRect);

        // Find out what state the button is on
        bool hoveredButton = (mousePositionX >= ButtonRect.xPos && mousePositionX <= ButtonRect.xPos + ButtonRect.width &&
                                mousePositionY >= ButtonRect.yPos && mousePositionY <= ButtonRect.yPos + ButtonRect.height)
                                && CursorIsInsideInputClip();
        if (hoveredButton) mouseHoveredOverWidgetThisFrame = true;
        bool clickedButton = hoveredButton && leftMouseButtonPressed;

        // Pick color based on state
        Color finalColor = buttonStyle.normalColor;
        float darkenAmount = 1.0f;
        if (hoveredButton)
        {
            finalColor = buttonStyle.hoveredColor;
            darkenAmount = hoverDarkenFactor;
        }
        if (clickedButton)
        {
            finalColor = buttonStyle.clickedColor;
            darkenAmount = activeDarkenFactor;
        }

        // Draw button with the colors
        GLTexture* buttonTexture = buttonStyle.imagePath.empty() ? nullptr : GetOrLoadTexture(buttonStyle.imagePath);
        DrawQuad(ButtonRect, finalColor, buttonTexture, darkenAmount);

        // If it's empty and don't do shit
        if (!ButtonTextItself.empty())
        {
            Font* font = GetOrLoadFont(buttonStyle.fontPath, (unsigned int)buttonStyle.textSize);
            if (font)
            {
                // Draw font but cut it off if it's too long and out of the button
                PushScissor(ButtonRect);

                // Center the label horizontally and roughly vertically inside the button
                float textWidth = font->measureTextWidth(ButtonTextItself, buttonStyle.textSize);
                float textHeight = font->measureTextHeight(ButtonTextItself, buttonStyle.textSize);

                float textX = ButtonRect.xPos + (ButtonRect.width - textWidth) * 0.5f;
                float centerYTopDown = ButtonRect.yPos + ButtonRect.height * 0.5f + textHeight * 0.5f;
                float bottomUpY = SCREEN_HEIGHT - centerYTopDown;

                DrawTextRaw(textX, bottomUpY, buttonStyle.textColor, buttonStyle.textSize, ButtonTextItself, font, 0.0f);

                PopScissor();
            }
        }

        return clickedButton;
    }
    bool IntSlider(Rect IntSliderRect, int& value, int minIntValue, int maxIntValue, int intStep,
        const SliderStyle& sliderStyle, std::string IntSliderTextItself)
    {
        if (!panelOpen) return false;
        SkipDraw(IntSliderRect);

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
        if (isActiveIntSlider)     handleDarkenAmount = activeDarkenFactor;
        else if (hoveredIntTrack)  handleDarkenAmount = hoverDarkenFactor;

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
        SkipDraw(FloatSliderRect);

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
        if (isActiveFloatSlider)     handleDarkenAmount = activeDarkenFactor;
        else if (hoveredFloatTrack)  handleDarkenAmount = hoverDarkenFactor;

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
        SkipDraw(SwitchRect);

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
        if (pressedSwitch)        switchDarkenAmount = activeDarkenFactor;
        else if (hoveredSwitch)   switchDarkenAmount = hoverDarkenFactor;

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
        SkipDraw(ImageRect);

        GLTexture* imageTexture = GetOrLoadTexture(imageStyle.imagePath);
        DrawQuad(ImageRect, imageStyle.color, imageTexture);
    }
    void Separator(Rect separatorRect, const SeparatorStyle& separatorStyle)
    {
        if (!panelOpen) return;
        SkipDraw(separatorRect);

        // i wish i was that long
        float separatorLineLength = 100000.0f;

        // The line passes through the center of the rect the caller gives us
        float lineCenterX = separatorRect.xPos + separatorRect.width * 0.5f;
        float lineCenterY = separatorRect.yPos + separatorRect.height * 0.5f;

        // Shift the quad left by half its length so its center sits on the point
        Rect longLine(lineCenterX - separatorLineLength * 0.5f,
                        lineCenterY - separatorStyle.thickness * 0.5f,
                        separatorLineLength,
                        separatorStyle.thickness,
                        separatorRect.rotation);

        DrawQuad(longLine, separatorStyle.lineColor, nullptr);
    }
    void Text(Rect TextRect, const TextStyle& textStyle, std::string TextItself)
    {
        if (!panelOpen) return;
        SkipDraw(TextRect);

        Font* font = GetOrLoadFont(textStyle.fontPath, (unsigned int)textStyle.size);
        if (!font) return;

        float bottomUpY = SCREEN_HEIGHT - TextRect.yPos;
        float angleRadians = glm::radians(TextRect.rotation);

        DrawTextRaw(TextRect.xPos, bottomUpY, textStyle.color, textStyle.size, TextItself, font, angleRadians);
    }
    bool MultiInputField(Rect inputFieldRect, std::string& textValue, const InputFieldStyle& inputFieldStyle)
    {
        if (!panelOpen) return false;
        SkipDraw(inputFieldRect);

        // Handles hover, focus and the background, and tells us if we should accept typing
        bool fieldIsFocused = BeginInputField(&textValue, inputFieldRect, inputFieldStyle.backGroundColor, inputFieldStyle.backgroundImagePath);

        // Each field remembers its own caret, keyed by the address of its text
        size_t& caretIndex = caretIndexMap[&textValue];

        // Edit the real string directly, new lines are allowed and every character is accepted
        bool textChanged = false;
        if (fieldIsFocused)
            textChanged = ApplyTypingToText(textValue, caretIndex, true, anyCharacterAllowed);

        DrawInputFieldText(inputFieldRect, textValue, caretIndex, fieldIsFocused, inputFieldStyle.textColor,
            inputFieldStyle.textSize, inputFieldStyle.fontPath, nullptr);
        return textChanged;
    }
    bool NumberInputField(Rect inputFieldRect, float& numberValue, const InputFieldStyle& inputFieldStyle)
    {
        if (!panelOpen) return false;
        SkipDraw(inputFieldRect);

        // Must be read before BeginInputField, because that call can change the focus
        bool fieldWasFocusedBefore = (focusedInputFieldIdentifier == &numberValue);
        bool fieldIsFocused = BeginInputField(&numberValue, inputFieldRect, inputFieldStyle.backGroundColor, inputFieldStyle.backgroundImagePath);

        size_t& caretIndex = caretIndexMap[&numberValue];

        // The text being typed lives here, because "3." or "-" is not a valid float yet
        std::string& editBuffer = numberFieldEditBufferMap[&numberValue];

        // On the frame the field gains focus, start the buffer from the current number
        if (fieldIsFocused && !fieldWasFocusedBefore)
            editBuffer = FormatNumberAsText(numberValue);

        // Edit the buffer (single line, digits . and - only), then try to turn it into a number
        bool numberChanged = false;
        if (fieldIsFocused && ApplyTypingToText(editBuffer, caretIndex, false, numberFieldAllowedCharacters))
        {
            char* parseEndPointer = nullptr;
            float parsedNumber = std::strtof(editBuffer.c_str(), &parseEndPointer);

            // Buffers like "" or "-" cannot be parsed yet, so the old number is kept until they can
            bool bufferIsValidNumber = !editBuffer.empty() && parseEndPointer != editBuffer.c_str();
            if (bufferIsValidNumber && parsedNumber != numberValue)
            {
                numberValue = parsedNumber;
                numberChanged = true;
            }
        }

        // While editing show the raw buffer, otherwise show the cleanly formatted number
        std::string unfocusedNumberText;
        if (!fieldIsFocused) unfocusedNumberText = FormatNumberAsText(numberValue);
        const std::string& textToShow = fieldIsFocused ? editBuffer : unfocusedNumberText;

        DrawInputFieldText(inputFieldRect, textToShow, caretIndex, fieldIsFocused, inputFieldStyle.textColor,
            inputFieldStyle.textSize, inputFieldStyle.fontPath, nullptr);
        return numberChanged;
    }
    bool CodeInputField(Rect inputFieldRect, std::string& textValue, const CodeInputFieldStyle& codeInputFieldStyle)
    {
        if (!panelOpen) return false;
        SkipDraw(inputFieldRect);

        bool fieldIsFocused = BeginInputField(&textValue, inputFieldRect, codeInputFieldStyle.backGroundColor, codeInputFieldStyle.backgroundImagePath);
        size_t& caretIndex = caretIndexMap[&textValue];

        // Same editing as MultiInputField, the only difference is how the text gets drawn
        bool textChanged = false;
        if (fieldIsFocused)
            textChanged = ApplyTypingToText(textValue, caretIndex, true, anyCharacterAllowed);

        // Passing the code style is what turns on the colored brackets
        DrawInputFieldText(inputFieldRect, textValue, caretIndex, fieldIsFocused, codeInputFieldStyle.textColor,
            codeInputFieldStyle.textSize, codeInputFieldStyle.fontPath, &codeInputFieldStyle);
        return textChanged;
    }
    void BeginScrollBox(Rect scrollBoxRect, float& scrollOffset, float contentHeight, const ScrollBoxStyle& scrollBoxStyle)
    {
        if (!panelOpen) return;
        SkipDraw(scrollBoxRect);
        scrollBoxOpen = true;

        // How far the content can move before its bottom reaches the bottom of the box
        float maximumScrollOffset = std::max(0.0f, contentHeight - scrollBoxRect.height);

        // ===[Background]===
        GLTexture* backgroundTexture = scrollBoxStyle.backgroundImagePath.empty()
            ? nullptr : GetOrLoadTexture(scrollBoxStyle.backgroundImagePath);
        DrawQuad(scrollBoxRect, scrollBoxStyle.backGroundColor, backgroundTexture);

        // ===[Scrollbar track sits on the right edge of the box]===
        Rect scrollBarTrackRect(scrollBoxRect.xPos + scrollBoxRect.width - scrollBarWidth,
                                scrollBoxRect.yPos, scrollBarWidth, scrollBoxRect.height);

        bool scrollBarTrackIsHovered = WidgetIsHovered(&scrollOffset, scrollBarTrackRect);
        if (scrollBarTrackIsHovered) mouseHoveredOverWidgetThisFrame = true;

        // ===[Handle size is proportional to how much of the content is visible]===
        float visibleFraction = (contentHeight > 0.0f) ? std::min(1.0f, scrollBoxRect.height / contentHeight) : 1.0f;
        float handleHeight = std::max(scrollBarMinimumHandleSize, scrollBarTrackRect.height * visibleFraction);
        handleHeight = std::min(handleHeight, scrollBarTrackRect.height);
        float handleTravelDistance = scrollBarTrackRect.height - handleHeight;

        // ===[Dragging the handle]===
        bool scrollBarIsActive = WidgetClaimsActive(&scrollOffset, scrollBarTrackIsHovered);
        if (scrollBarIsActive && leftMouseButtonDown && handleTravelDistance > 0.0f)
        {
            float normalizedDragPosition = (float)(mousePositionY - scrollBarTrackRect.yPos - handleHeight * 0.5f) / handleTravelDistance;
            scrollOffset = std::clamp(normalizedDragPosition, 0.0f, 1.0f) * maximumScrollOffset;
        }

        // ===[Mouse wheel while the cursor is anywhere over the box]===
        bool cursorIsInsideScrollBox = (mousePositionX >= scrollBoxRect.xPos && mousePositionX <= scrollBoxRect.xPos + scrollBoxRect.width &&
                                        mousePositionY >= scrollBoxRect.yPos && mousePositionY <= scrollBoxRect.yPos + scrollBoxRect.height);
        if (cursorIsInsideScrollBox)
            scrollOffset -= (float)scrollDeltaYState * mouseWheelScrollPixels;  // Wheel up = content moves down

        scrollOffset = std::clamp(scrollOffset, 0.0f, maximumScrollOffset);

        // ===[Draw the track and the handle]===
        GLTexture* barTexture    = scrollBoxStyle.barImagePath.empty()    ? nullptr : GetOrLoadTexture(scrollBoxStyle.barImagePath);
        GLTexture* scrollTexture = scrollBoxStyle.scrollImagePath.empty() ? nullptr : GetOrLoadTexture(scrollBoxStyle.scrollImagePath);

        float scrollProgress = (maximumScrollOffset > 0.0f) ? scrollOffset / maximumScrollOffset : 0.0f;
        Rect scrollBarHandleRect(scrollBarTrackRect.xPos, scrollBarTrackRect.yPos + scrollProgress * handleTravelDistance,
                                 scrollBarWidth, handleHeight);

        float handleDarkenAmount = 1.0f;
        if (scrollBarIsActive)               handleDarkenAmount = activeDarkenFactor;
        else if (scrollBarTrackIsHovered)    handleDarkenAmount = hoverDarkenFactor;

        DrawQuad(scrollBarTrackRect, scrollBoxStyle.barColor, barTexture);
        Color finalHandleColor = scrollTexture ? scrollBoxStyle.scrollColor : ApplyDarken(scrollBoxStyle.scrollColor, handleDarkenAmount);
        DrawQuad(scrollBarHandleRect, finalHandleColor, scrollTexture, handleDarkenAmount);

        // ===[Clip drawing and mouse input to the content area, so nothing covers or reacts through the scrollbar]===
        Rect scrollContentRect(scrollBoxRect.xPos, scrollBoxRect.yPos, scrollBoxRect.width - scrollBarWidth, scrollBoxRect.height);
        PushScissor(scrollContentRect);

        inputClipRect = scrollContentRect;
        inputClipIsActive = true;
    }
    void EndScrollBox()
    {
        scrollBoxOpen = false;
        inputClipIsActive = false;
        PopScissor();
    }
    void BeginDropDown(Rect dropDownRect, int& selectedIndex, const std::vector<std::string>& labelList,
                        const DropDownStyle& dropDownStyle, std::string placeholderText)
    {
        if (!panelOpen) return;
        SkipDraw(dropDownRect);
        dropDownOpen = true;

        int itemCount = (int)labelList.size();
        float itemHeight = dropDownRect.height;
        bool dropDownIsExpanded = (expandedDropDownIdentifier == &selectedIndex);

        // ===[Header: click it to expand or collapse the list]===
        bool headerIsHovered = WidgetIsHovered(&selectedIndex, dropDownRect);
        if (headerIsHovered) mouseHoveredOverWidgetThisFrame = true;

        if (headerIsHovered && leftMouseButtonPressed)
        {
            dropDownIsExpanded = !dropDownIsExpanded;
            expandedDropDownIdentifier = dropDownIsExpanded ? &selectedIndex : nullptr;
        }

        // ===[Draw the header with the selected label, or the placeholder if nothing is selected]===
        float headerDarkenAmount = 1.0f;
        if (headerIsHovered && leftMouseButtonDown)  headerDarkenAmount = activeDarkenFactor;
        else if (headerIsHovered)                    headerDarkenAmount = hoverDarkenFactor;

        GLTexture* headerTexture = dropDownStyle.backgroundImagePath.empty()
            ? nullptr : GetOrLoadTexture(dropDownStyle.backgroundImagePath);
        Color finalHeaderColor = headerTexture ? dropDownStyle.backGroundColor
                                               : ApplyDarken(dropDownStyle.backGroundColor, headerDarkenAmount);
        DrawQuad(dropDownRect, finalHeaderColor, headerTexture, headerDarkenAmount);

        Font* font = GetOrLoadFont(dropDownStyle.fontPath, (unsigned int)dropDownStyle.textSize);

        bool selectedIndexIsValid = (selectedIndex >= 0 && selectedIndex < itemCount);
        const std::string& headerText = selectedIndexIsValid ? labelList[selectedIndex] : placeholderText;

        if (font && !headerText.empty())
        {
            PushScissor(dropDownRect);

            float headerTextHeight = font->measureTextHeight(headerText, dropDownStyle.textSize);
            float headerCenterYTopDown = dropDownRect.yPos + dropDownRect.height * 0.5f + headerTextHeight * 0.5f;
            DrawTextRaw(dropDownRect.xPos + inputFieldPadding, SCREEN_HEIGHT - headerCenterYTopDown,
                        dropDownStyle.textColor, dropDownStyle.textSize, headerText, font, 0.0f);

            PopScissor();
        }

        if (!dropDownIsExpanded) return;

        // ===[List sits right under the header and shows at most maxVisibleItems rows]===
        int visibleItemCount = std::min(itemCount, std::max(1, dropDownStyle.maxVisibleItems));
        Rect listRect(dropDownRect.xPos, dropDownRect.yPos + dropDownRect.height,
                      dropDownRect.width, visibleItemCount * itemHeight);

        float contentHeight = itemCount * itemHeight;
        float maximumScrollOffset = std::max(0.0f, contentHeight - listRect.height);
        bool listNeedsScrollBar = maximumScrollOffset > 0.0f;

        float& scrollOffset = dropDownScrollOffsetMap[&selectedIndex];

        // ===[List background]===
        GLTexture* listTexture = dropDownStyle.backgroundImagePath.empty()
            ? nullptr : GetOrLoadTexture(dropDownStyle.backgroundImagePath);
        DrawQuad(listRect, dropDownStyle.dropcBackGroundColor, listTexture);

        // ===[Scrollbar track on the right edge, only when the list is longer than what is visible]===
        Rect scrollBarTrackRect(listRect.xPos + listRect.width - scrollBarWidth, listRect.yPos,
                                scrollBarWidth, listRect.height);

        bool scrollBarTrackIsHovered = listNeedsScrollBar && WidgetIsHovered(&scrollOffset, scrollBarTrackRect);
        if (scrollBarTrackIsHovered) mouseHoveredOverWidgetThisFrame = true;

        float visibleFraction = (contentHeight > 0.0f) ? std::min(1.0f, listRect.height / contentHeight) : 1.0f;
        float handleHeight = std::max(scrollBarMinimumHandleSize, scrollBarTrackRect.height * visibleFraction);
        handleHeight = std::min(handleHeight, scrollBarTrackRect.height);
        float handleTravelDistance = scrollBarTrackRect.height - handleHeight;

        // ===[Dragging the handle]===
        bool scrollBarIsActive = listNeedsScrollBar && WidgetClaimsActive(&scrollOffset, scrollBarTrackIsHovered);
        if (scrollBarIsActive && leftMouseButtonDown && handleTravelDistance > 0.0f)
        {
            float normalizedDragPosition = (float)(mousePositionY - scrollBarTrackRect.yPos - handleHeight * 0.5f) / handleTravelDistance;
            scrollOffset = std::clamp(normalizedDragPosition, 0.0f, 1.0f) * maximumScrollOffset;
        }

        // ===[Mouse wheel while the cursor is anywhere over the list]===
        bool cursorIsInsideList = (mousePositionX >= listRect.xPos && mousePositionX <= listRect.xPos + listRect.width &&
                                   mousePositionY >= listRect.yPos && mousePositionY <= listRect.yPos + listRect.height);
        if (cursorIsInsideList)
            scrollOffset -= (float)scrollDeltaYState * mouseWheelScrollPixels;

        scrollOffset = std::clamp(scrollOffset, 0.0f, maximumScrollOffset);

        // ===[Items are clipped to the list area, minus the scrollbar if there is one]===
        Rect listContentRect(listRect.xPos, listRect.yPos,
                             listRect.width - (listNeedsScrollBar ? scrollBarWidth : 0.0f), listRect.height);

        bool cursorIsInsideListContent = (mousePositionX >= listContentRect.xPos && mousePositionX <= listContentRect.xPos + listContentRect.width &&
                                          mousePositionY >= listContentRect.yPos && mousePositionY <= listContentRect.yPos + listContentRect.height);

        bool anItemWasClicked = false;
        PushScissor(listContentRect);

        // Only walk the rows that can actually be seen
        int firstVisibleItemIndex = std::max(0, (int)(scrollOffset / itemHeight));
        int lastVisibleItemIndex = std::min(itemCount - 1, firstVisibleItemIndex + visibleItemCount);

        for (int itemIndex = firstVisibleItemIndex; itemIndex <= lastVisibleItemIndex; itemIndex++)
        {
            Rect itemRect(listContentRect.xPos, listRect.yPos + itemIndex * itemHeight - scrollOffset,
                          listContentRect.width, itemHeight);

            // Rows scrolled half out of view must not react outside the visible list
            bool itemIsHovered = cursorIsInsideListContent && WidgetIsHovered(&labelList[itemIndex], itemRect);
            if (itemIsHovered) mouseHoveredOverWidgetThisFrame = true;

            if (itemIsHovered && leftMouseButtonPressed)
            {
                selectedIndex = itemIndex;
                anItemWasClicked = true;
            }

            // Highlight hovered rows, and keep the selected row slightly darker
            float itemDarkenAmount = 1.0f;
            if (itemIsHovered && leftMouseButtonDown)    itemDarkenAmount = activeDarkenFactor;
            else if (itemIsHovered)                      itemDarkenAmount = hoverDarkenFactor;
            else if (itemIndex == selectedIndex)         itemDarkenAmount = hoverDarkenFactor;

            if (itemDarkenAmount != 1.0f)
                DrawQuad(itemRect, ApplyDarken(dropDownStyle.dropcBackGroundColor, itemDarkenAmount), nullptr);

            if (font && !labelList[itemIndex].empty())
            {
                float itemTextHeight = font->measureTextHeight(labelList[itemIndex], dropDownStyle.textSize);
                float itemCenterYTopDown = itemRect.yPos + itemRect.height * 0.5f + itemTextHeight * 0.5f;
                DrawTextRaw(itemRect.xPos + inputFieldPadding, SCREEN_HEIGHT - itemCenterYTopDown,
                            dropDownStyle.textColor, dropDownStyle.textSize, labelList[itemIndex], font, 0.0f);
            }
        }
        PopScissor();

        // ===[Draw the track and the handle, same look as the scroll box]===
        if (listNeedsScrollBar)
        {
            GLTexture* barTexture    = dropDownStyle.barImagePath.empty()    ? nullptr : GetOrLoadTexture(dropDownStyle.barImagePath);
            GLTexture* scrollTexture = dropDownStyle.scrollImagePath.empty() ? nullptr : GetOrLoadTexture(dropDownStyle.scrollImagePath);

            float scrollProgress = scrollOffset / maximumScrollOffset;
            Rect scrollBarHandleRect(scrollBarTrackRect.xPos, scrollBarTrackRect.yPos + scrollProgress * handleTravelDistance,
                                     scrollBarWidth, handleHeight);

            float handleDarkenAmount = 1.0f;
            if (scrollBarIsActive)               handleDarkenAmount = activeDarkenFactor;
            else if (scrollBarTrackIsHovered)    handleDarkenAmount = hoverDarkenFactor;

            DrawQuad(scrollBarTrackRect, DefaultClickedColor, barTexture);
            Color finalHandleColor = scrollTexture ? DefaultHoverColor : ApplyDarken(DefaultHoverColor, handleDarkenAmount);
            DrawQuad(scrollBarHandleRect, finalHandleColor, scrollTexture, handleDarkenAmount);
        }

        // ===[Picking an item closes the list, and so does clicking anywhere outside the header and list]===
        bool clickedOutside = leftMouseButtonPressed && !headerIsHovered && !cursorIsInsideList;
        if (anItemWasClicked || clickedOutside)
            expandedDropDownIdentifier = nullptr;
    }
    void EndDropDown()
    {
        // Everything was clipped and drawn inside BeginDropDown, so there is no scissor left to pop
        dropDownOpen = false;
    }
    void ToolTip(Rect toolTipRect, const ToolTipStyle& toolTipStyle, std::string tipText)
    {
        if (!panelOpen || tipText.empty()) return;

        // Only show while the mouse is inside the area this tooltip belongs to
        bool mouseIsInsideHoverArea = mousePositionX >= toolTipRect.xPos && mousePositionX <= toolTipRect.xPos + toolTipRect.width &&
                                      mousePositionY >= toolTipRect.yPos && mousePositionY <= toolTipRect.yPos + toolTipRect.height;
        if (!mouseIsInsideHoverArea || !CursorIsInsideInputClip()) return;

        Font* tipFont = GetOrLoadFont(toolTipStyle.fontPath, (unsigned int)toolTipStyle.textSize);
        if (!tipFont) return;

        // Box is the text size plus padding, placed just below and right of the mouse
        float tipTextWidth  = tipFont->measureTextWidth(tipText, toolTipStyle.textSize);
        float tipTextHeight = tipFont->measureTextHeight(tipText, toolTipStyle.textSize);
        Rect tipBoxRect((float)mousePositionX + 16.0f, (float)mousePositionY + 16.0f,
                        tipTextWidth + inputFieldPadding * 2.0f, tipTextHeight + inputFieldPadding * 2.0f);

        // Turn the panel scissor off so the tooltip is not cut at the panel edge
        glDisable(GL_SCISSOR_TEST);

        GLTexture* toolTipTexture = toolTipStyle.backgroundImagePath.empty() ? nullptr : GetOrLoadTexture(toolTipStyle.backgroundImagePath);
        DrawQuad(tipBoxRect, toolTipStyle.backGroundColor, toolTipTexture, 0.0f);

        // Text position is measured from the bottom of the screen
        float tipBaselineBottomUpY = SCREEN_HEIGHT - (tipBoxRect.yPos + inputFieldPadding + tipTextHeight);
        DrawTextRaw(tipBoxRect.xPos + inputFieldPadding, tipBaselineBottomUpY,
                    toolTipStyle.textColor, toolTipStyle.textSize, tipText, tipFont, 0.0f);

        // Turn the scissor back on for whatever draws next
        glEnable(GL_SCISSOR_TEST);
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
