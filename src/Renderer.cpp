#include "Renderer.hpp"

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
        bool leftMouseButtonPrssed = false;
        bool rightMouseButtonPrssed = false;
        bool middleMouseButtonPrssed = false;

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
    }
    void EndPanel()
    {
        panelOpen = false;
    }

    // |=====================================================
    // |---[Common UI stuff]---------------------------------
    // |=====================================================
    bool DrawButton(Rect ButtonRect, Color ButtonColor, std::string ButtonImagePath)
    {
        if (!panelOpen) return false;

        GLTexture* tex = GetOrLoadTexture(ButtonImagePath);
        DrawQuad(ButtonRect, ButtonColor, tex);

        return true;
    }
    void DrawSlider(Rect SliderRect, Color SliderColor)
    {

    }
    void DrawSwitch(Rect SwitchRect, Color SwitchColor)
    {

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
