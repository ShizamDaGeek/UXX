#ifndef FONT_HPP
#define FONT_HPP

// The Font class will help init, draw and delete font

#include <glad/glad.h>
#include <GL/gl.h>
#include <map>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

    void rendererText(Shader shader, const std::string& text, float positionX, float positionY,
        float scale, unsigned int VAO, unsigned int VBO, GLint modelLoc, float angleRadians = 0.0f);

    float measureTextWidth(const std::string& text, float scale);
    float measureTextHeight(const std::string& text, float scale);

    void deleteFreeType();

private:
    std::map<char, Character> characters;
};

#endif
