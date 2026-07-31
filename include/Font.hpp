#ifndef FONT_HPP
#define FONT_HPP

#include <glad/glad.h>
#include <map>
#include <iostream>

#include "Shader.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

struct Character
{
    unsigned int textureID;
    int width, height;
    int bearingX, bearingY;
    long advance;
};

class Font
{
public:
    bool initFreeType(const char* fontpath, unsigned int pixelHeight);
    void rendererText(Shader shader, const std::string& text, float x, float y, float scale,
        unsigned int VAO, unsigned int VBO);
    void deleteFreeType();
private:
    std::map<char, Character> characters;
};

#endif
