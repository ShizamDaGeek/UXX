#include "Renderer.hpp"
#include <GLFW/glfw3.h>

namespace Renderer
{
    // |=====================================================
    // |---[Private state]-----------------------------------
    // |=====================================================
    namespace
    {
        VAO* vao = nullptr;
        VBO* vbo = nullptr;
        EBO* ebo = nullptr;
        Shader* shader = nullptr;

        std::unordered_map<std::string, GLTexture*> textureCache;
        GLTexture* GetOrLoadTexture(const std::string& path);

        bool panelOpen = false;
        bool leftMouseButtonPressed = false;
        bool rightMouseButtonPressed = false;
        bool middleMouseButtonPressed = false;

        double mousePositionX, mousePositionY;

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
        // |---[Helper Functions]--------------------------------
        // |=====================================================
        GLTexture* GetOrLoadTexture(const std::string& path)
        {
            auto it = textureCache.find(path);
            if (it != textureCache.end()) return it->second;

            GLTexture* tex = new GLTexture(path.c_str(), "", 0);
            tex->texUnit(*shader, "tex0", 0);
            textureCache[path] = tex;

            return tex;
        }
        void DrawQuad(Rect rect, Color color, GLTexture* tex)
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
            glUniform1i(glGetUniformLocation(shader->ID, "uUseTexture"), tex ? 1 : 0);

            if (tex) tex->Bind();

            vao->Bind();
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            vao->Unbind();

            if (tex) tex->Unbind();
        }
        // Shared drag math for sliders
        float SliderDragT(Rect SliderRect, float handleWidth)
        {
            bool hoveringTrack = (mousePositionX >= SliderRect.xPos && mousePositionX <= SliderRect.xPos + SliderRect.width &&
                                    mousePositionY >= SliderRect.yPos && mousePositionY <= SliderRect.yPos + SliderRect.height);

            if (!leftMouseButtonPressed || !hoveringTrack) return -1.0f;

            float newT = (float)(mousePositionX - SliderRect.xPos - handleWidth * 0.5f) / (SliderRect.width - handleWidth);
            return std::clamp(newT, 0.0f, 1.0f);
        }
    }

    // |=====================================================
    // |---[Helper Functions]--------------------------------
    // |=====================================================
    void SetMouseState(double x, double y, bool leftDown, bool rightDown, bool middleDown)
    {
        mousePositionX = x;
        mousePositionY = y;
        leftMouseButtonPressed = leftDown;
        rightMouseButtonPressed = rightDown;
        middleMouseButtonPressed = middleDown;
    }

    // |=====================================================
    // |---[Init]--------------------------------------------
    // |=====================================================
    void init()
    {
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
    }

    // |=====================================================
    // |---[Begin and End Panel]-----------------------------
    // |=====================================================
    void BeginPanel(Rect PanelRect, Color PanelColor)
    {
        panelOpen = true;
        DrawQuad(PanelRect, PanelColor, nullptr);

        GLint scissorX = (GLint)(PanelRect.xPos * SCREEN_SCALE_X);
        GLint scissorY = (GLint)((SCREEN_HEIGHT - (PanelRect.yPos + PanelRect.height)) * SCREEN_SCALE_Y);
        GLsizei scissorW = (GLsizei)(PanelRect.width  * SCREEN_SCALE_X);
        GLsizei scissorH = (GLsizei)(PanelRect.height * SCREEN_SCALE_Y);

        glEnable(GL_SCISSOR_TEST);
        glScissor(scissorX, scissorY, scissorW, scissorH);
    }
    void EndPanel()
    {
        panelOpen = false;
    }

    // |=====================================================
    // |---[Common UI stuff]---------------------------------
    // |=====================================================
    bool DrawButton(Rect ButtonRect, Color ButtonColor, Color ButtonHoverColor, Color ButtonClickedColor, std::string ButtonImagePath)
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

        return clicked;
    }
    bool DrawIntSlider(Rect IntSliderRect, Color IntTrackColor, Color IntHandleColor, int& value, int minIntValue, int maxIntValue, int intStep)
    {
        if (!panelOpen) return false;

        float handleWidth = IntSliderRect.height;
        float t = (float)(value - minIntValue) / (float)(maxIntValue - minIntValue);
        float handleX = IntSliderRect.xPos + t * (IntSliderRect.width - handleWidth);
        Rect handleRect{ handleX, IntSliderRect.yPos, handleWidth, IntSliderRect.height, 0.0f };

        bool changed = false;
        float newT = SliderDragT(IntSliderRect, handleWidth);
        if (newT >= 0.0f)
        {
            int steps = (maxIntValue - minIntValue) / std::max(intStep, 1);
            int newValue = minIntValue + (int)std::round(newT * steps) * intStep;
            newValue = std::clamp(newValue, minIntValue, maxIntValue);
            if (newValue != value)
            {
                value = newValue;
                changed = true;
            }
        }

        DrawQuad(IntSliderRect, IntTrackColor, nullptr);
        DrawQuad(handleRect, IntHandleColor, nullptr);

        return changed;
    }
    bool DrawFloatSlider(Rect FloatSliderRect, Color FloatTrackColor, Color FloatHandleColor, float& value, float minFloatValue, float maxFloatValue)
    {
        if (!panelOpen) return false;

        float handleWidth = FloatSliderRect.height;
        float t = (value - minFloatValue) / (maxFloatValue - minFloatValue);
        float handleX = FloatSliderRect.xPos + t * (FloatSliderRect.width - handleWidth);
        Rect handleRect{ handleX, FloatSliderRect.yPos, handleWidth, FloatSliderRect.height, 0.0f };

        bool changed = false;
        float newT = SliderDragT(FloatSliderRect, handleWidth);
        if (newT >= 0.0f)
        {
            float newValue = minFloatValue + newT * (maxFloatValue - minFloatValue);
            if (newValue != value)
            {
                value = newValue;
                changed = true;
            }
        }

        DrawQuad(FloatSliderRect, FloatTrackColor, nullptr);
        DrawQuad(handleRect, FloatHandleColor, nullptr);

        return changed;
    }
    bool DrawSwitch(Rect SwitchRect, Color OnColor, Color OffColor, bool& value)
    {
        if (!panelOpen) return false;

        bool hovered = (mousePositionX >= SwitchRect.xPos && mousePositionX <= SwitchRect.xPos + SwitchRect.width &&
                        mousePositionY >= SwitchRect.yPos && mousePositionY <= SwitchRect.yPos + SwitchRect.height);

        bool toggled = false;
        if (hovered && leftMouseButtonPressed)
        {
            value = !value;
            toggled = true;
        }

        DrawQuad(SwitchRect, value ? OnColor : OffColor, nullptr);

        return toggled;
    }

    // |=====================================================
    // |---[Advanced UI stuff]-------------------------------
    // |=====================================================
    void DrawImage(Rect ImageRect, Color ImageColor, std::string ImagePath)
    {
        if (!panelOpen) return;

        GLTexture* tex = GetOrLoadTexture(ImagePath);
        DrawQuad(ImageRect, ImageColor, tex);
    }

    // |=====================================================
    // |---[ALL OUR FOOD KEEPS BLOWING UP]-------------------
    // |=====================================================
    void BlowUp()
    {
        for (auto& [path, tex] : textureCache)
        {
            tex->Delete();
            delete tex;
        }
        textureCache.clear();
    }
}
