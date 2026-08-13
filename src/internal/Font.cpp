#include "internal/Font.hpp"

// |=====================================================
// |---[Init FreeType]-----------------------------------
// |=====================================================
bool Font::initFreeType(const char* fontpath, unsigned int pixelHeight)
{
    FT_Library freetypeLibrary;
    if (FT_Init_FreeType(&freetypeLibrary))
    {
        std::cout << "Failed to init FreeType\n";
        return false;
    }

    FT_Face fontFace;
    if (FT_New_Face(freetypeLibrary, fontpath, 0, &fontFace))
    {
        std::cout << "Failed to load font: " << fontpath << "\n";
        FT_Done_FreeType(freetypeLibrary);
        return false;
    }

    // Rasterize glyphs at the requested pixel height
    FT_Set_Pixel_Sizes(fontFace, 0, pixelHeight);
    // disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // ===[Load the first 128 ASCII glyphs into individual textures]===
    for (unsigned char charCode = 0; charCode < 128; charCode++)
    {
        if (FT_Load_Char(fontFace, charCode, FT_LOAD_RENDER))
        {
            std::cout << "Failed to load glyph: " << charCode << "\n";
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RED,
            fontFace->glyph->bitmap.width, fontFace->glyph->bitmap.rows,
            0, GL_RED, GL_UNSIGNED_BYTE,
            fontFace->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Cache this glyph's texture and metrics for later measurement/drawing
        Character character =
        {
            texture,
            (int)fontFace->glyph->bitmap.width,
            (int)fontFace->glyph->bitmap.rows,
            fontFace->glyph->bitmap_left,
            fontFace->glyph->bitmap_top,
            fontFace->glyph->advance.x
        };
        characters.insert({ (char)charCode, character });
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(fontFace);
    FT_Done_FreeType(freetypeLibrary);

    return true;
}

// |=====================================================
// |---[Renderer Text]-----------------------------------
// |=====================================================
void Font::rendererText(Shader shader, const std::string& text, float positionX, float positionY,
    float scale, unsigned int VAO, unsigned int VBO, float angleRadians)
{
    shader.Use();

    // Build model matrix. Translate to (x,y), then rotate around that point
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(positionX, positionY, 0.0f));
    model = glm::rotate(model, angleRadians, glm::vec3(0.0f, 0.0f, 1.0f));

    // upload it
    GLint modelLocation = glGetUniformLocation(shader.ID, "model");
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Unroatated local space
    float penX = 0.0f;

    static bool printedOnce = false;
    for (char currentChar : text)
    {
        if (characters.find(currentChar) == characters.end())
        {
            if (!printedOnce) std::cout << "[Font] char '" << currentChar << "' not found in glyph map\n";
            continue;
        }

        Character glyph = characters[currentChar];

        // local-space position, relative to pen origin (0,0)
        float glyphX        = penX + glyph.bearingX * scale;
        float glyphY        = -(glyph.height - glyph.bearingY) * scale;
        float glyphWidth    = glyph.width * scale;
        float glyphHeight   = glyph.height * scale;

        if (!printedOnce)
        {
            std::cout << "[Font] char '" << currentChar << "' tex=" << glyph.textureID
                        << " xpos=" << glyphX << " ypos=" << glyphY
                        << " w=" << glyphWidth << " h=" << glyphHeight << "\n";
        }

        // Quad corners in the order the two triangles expect
        float vertices[6][4] =
        {
            { glyphX,              glyphY + glyphHeight,   0.0f, 0.0f },
            { glyphX,              glyphY,                 0.0f, 1.0f },
            { glyphX + glyphWidth, glyphY,                 1.0f, 1.0f },

            { glyphX,              glyphY + glyphHeight,   0.0f, 0.0f },
            { glyphX + glyphWidth, glyphY,                 1.0f, 1.0f },
            { glyphX + glyphWidth, glyphY + glyphHeight,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, glyph.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR && !printedOnce)
            std::cout << "[Font] GL error after draw: 0x" << std::hex << error << std::dec << "\n";

        // Move the pen forward by this glyph's advance (26.6 fixed-point, hence >> 6)
        penX += (glyph.advance >> 6) * scale;
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
    for (char currentChar : text)
    {
        auto foundChar = characters.find(currentChar);
        if (foundChar == characters.end()) continue;

        width += (foundChar->second.advance >> 6) * scale;
    }

    return width;
}
float Font::measureTextHeight(const std::string& text, float scale)
{
    float minY = 0.0f, maxY = 0.0f;
    bool first = true;

    // Track the tallest ascender and lowest descender across all glyphs in the string
    for (char currentChar : text)
    {
        auto foundChar = characters.find(currentChar);
        if (foundChar == characters.end()) continue;

        const Character& glyph = foundChar->second;

        // Mirrors the vertex math in rendererText, top edge sits at bearingY, bottom edge sits at -(height - bearingY)
        float top = glyph.bearingY * scale;
        float bottom = -(glyph.height - glyph.bearingY) * scale;

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
    for (auto& [charCode, glyph] : characters)
        glDeleteTextures(1, &glyph.textureID);

    characters.clear();
}
