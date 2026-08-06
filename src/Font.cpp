#include "Font.hpp"

// |=====================================================
// |---[Init FreeType]-----------------------------------
// |=====================================================
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

    // Rasterize glyphs at the requested pixel height
    FT_Set_Pixel_Sizes(face, 0, pixelHeight);

    // disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // ===[Load the first 128 ASCII glyphs into individual textures]===
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

        // Cache this glyph's texture and metrics for later measurement/drawing
        Character character =
        {
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

// |=====================================================
// |---[Renderer Text]-----------------------------------
// |=====================================================
void Font::rendererText(Shader shader, const std::string& text, float x, float y, float scale,
    unsigned int VAO, unsigned int VBO, float angleRadians)
{
    shader.Use();

    // Build model matrix. Translate to (x,y), then rotate around that point
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
    model = glm::rotate(model, angleRadians, glm::vec3(0.0f, 0.0f, 1.0f));

    // upload it
    GLint modelLoc = glGetUniformLocation(shader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Unroatated local space
    float penX = 0.0f;

    static bool printedOnce = false;
    for (char c : text)
    {
        if (characters.find(c) == characters.end())
        {
            if (!printedOnce) std::cout << "[Font] char '" << c << "' not found in glyph map\n";
            continue;
        }

        Character ch = characters[c];

        // local-space position, relative to pen origin (0,0)
        float xpos = penX + ch.bearingX * scale;
        float ypos = -(ch.height - ch.bearingY) * scale;
        float w = ch.width * scale;
        float h = ch.height * scale;

        if (!printedOnce)
        {
            std::cout << "[Font] char '" << c << "' tex=" << ch.textureID
                        << " xpos=" << xpos << " ypos=" << ypos
                        << " w=" << w << " h=" << h << "\n";
        }

        // Quad corners in the order the two triangles expect
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

        // Move the pen forward by this glyph's advance (26.6 fixed-point, hence >> 6)
        penX += (ch.advance >> 6) * scale;
    }

    printedOnce = true;
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// |=====================================================
// |---[Measure the Width&height of the Text]------------
// |=====================================================
float Font::measureTextWidth(const std::string& text, float scale)
{
    float width = 0.0f;

    // Sum each glyph's horizontal advance to get the total rendered width
    for (char c : text)
    {
        auto it = characters.find(c);
        if (it == characters.end()) continue;

        width += (it->second.advance >> 6) * scale;
    }

    return width;
}
float Font::measureTextHeight(const std::string& text, float scale)
{
    float minY = 0.0f, maxY = 0.0f;
    bool first = true;

    // Track the tallest ascender and lowest descender across all glyphs in the string
    for (char c : text)
    {
        auto it = characters.find(c);
        if (it == characters.end()) continue;

        const Character& ch = it->second;

        // Mirrors the vertex math in rendererText, top edge sits at bearingY, bottom edge sits at -(height - bearingY)
        float top = ch.bearingY * scale;
        float bottom = -(ch.height - ch.bearingY) * scale;

        if (first)
        {
            minY = bottom;
            maxY = top;
            first = false;
        }
        else
        {
            minY = std::min(minY, bottom);
            maxY = std::max(maxY, top);
        }
    }

    return maxY - minY;
}

// |=====================================================
// |---[I just installed it why I gotta delete it]-------
// |=====================================================
void Font::deleteFreeType()
{
    // Free every cached glyph texture before dropping the map
    for (auto& [c, ch] : characters)
        glDeleteTextures(1, &ch.textureID);

    characters.clear();
}
