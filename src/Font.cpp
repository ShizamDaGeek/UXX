#include "Font.hpp"
#include <GL/gl.h>

bool Font::initFreeType(const char* fontpath, unsigned int pixelHeight)
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "Failed to init FreeType\n";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontpath, 0, &face))
    {
        std::cout << "Failed to load font: " << fontpath << "\n";
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, pixelHeight);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    for (unsigned char c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "Failed to load glyph: " << c << "\n";
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RED,
            face->glyph->bitmap.width, face->glyph->bitmap.rows,
            0, GL_RED, GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        Character character = {
            texture,
            (int)face->glyph->bitmap.width,
            (int)face->glyph->bitmap.rows,
            face->glyph->bitmap_left,
            face->glyph->bitmap_top,
            face->glyph->advance.x
        };
        characters.insert({ (char)c, character });
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
}
void Font::rendererText(Shader shader, const std::string& text, float x, float y, float scale,
    unsigned int VAO, unsigned int VBO)
{
    shader.Use();
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    static bool printedOnce = false;

    for (char c : text)
    {
        if (characters.find(c) == characters.end())
        {
            if (!printedOnce) std::cout << "[Font] char '" << c << "' not found in glyph map\n";
            continue;
        }

        Character ch = characters[c];

        float xpos = x + ch.bearingX * scale;
        float ypos = y - (ch.height - ch.bearingY) * scale;

        float w = ch.width * scale;
        float h = ch.height * scale;

        if (!printedOnce)
        {
            std::cout << "[Font] char '" << c << "' tex=" << ch.textureID
                        << " xpos=" << xpos << " ypos=" << ypos
                        << " w=" << w << " h=" << h << "\n";
        }

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR && !printedOnce)
            std::cout << "[Font] GL error after draw: 0x" << std::hex << err << std::dec << "\n";

        x += (ch.advance >> 6) * scale;
    }

    printedOnce = true;

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
void Font::deleteFreeType()
{
    for (auto& [c, ch] : characters)
        glDeleteTextures(1, &ch.textureID);

    characters.clear();
}
