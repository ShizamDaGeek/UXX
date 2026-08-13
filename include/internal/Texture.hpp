#ifndef TEXTURE_HPP
#define TEXTURE_HPP

// The Texture class will help with texture (isn't it obvious)

#include <glad/glad.h>
#include <GL/gl.h>
#include <iostream>
#include <stb_image.h>
#include <string>
#include <stdexcept>
#include <cstring>

#include "Shader.hpp"

struct Texture
{
    unsigned int id;
    std::string type;
    std::string path;
};


class GLTexture
{
public:
    unsigned int ID;
    const char* type;
    unsigned int unit;

    bool hasAlpha = false;
    bool isMissingFallback = false;

    static std::string missingTexturePath;

    GLTexture(const char* image, const char* texType, unsigned int slot);

    static unsigned int loadTexture(const std::string& path, const char* texType,
        bool* outHasAlpha = nullptr, bool* outIsFallback = nullptr);

    void texUnit(Shader& shader, const char* uniform, unsigned int unit);
    void Bind();
    void Unbind();
    void Delete();

    static void EnableTransparency();
    static void DisableTransparency();
};

#endif
