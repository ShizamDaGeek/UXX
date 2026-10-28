#include "internal/Texture.hpp"

std::string GLTexture::missingTexturePath = "../UXXAssets/Images/missing_texture.png";

namespace
{
    // Does the actual GL upload once we have valid pixel data (real or fallback).
    unsigned int uploadTexture(unsigned char* imageBytes, int imageWidth, int imageHeight,
        int numberOfColorChannel, const char* textureType, bool& hasAlphaOut)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        hasAlphaOut = (numberOfColorChannel == 4);

        if (strcmp(textureType, "normal") == 0)
        {
            if (numberOfColorChannel == 4)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBytes);
            else if (numberOfColorChannel == 3)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBytes);
            else
                throw std::invalid_argument("Normal Texture type recognition failed!");
        }
        else if (strcmp(textureType, "displacement") == 0)
        {
            if (numberOfColorChannel == 4)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBytes);
            else if (numberOfColorChannel == 3)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBytes);
            else if (numberOfColorChannel == 1)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imageWidth, imageHeight, 0, GL_RED, GL_UNSIGNED_BYTE, imageBytes);
            else
                throw std::invalid_argument("Displacement Texture type recognition failed!");
        }
        else if (strcmp(textureType, "specular") == 0)
        {
            if (numberOfColorChannel == 4)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBytes);
            else if (numberOfColorChannel == 3)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBytes);
            else if (numberOfColorChannel == 1)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imageWidth, imageHeight, 0, GL_RED, GL_UNSIGNED_BYTE, imageBytes);
            else
                throw std::invalid_argument("Specular Texture type recognition failed!");
        }
        else
        {
            if (numberOfColorChannel == 4)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBytes);
            else if (numberOfColorChannel == 3)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBytes);
            else if (numberOfColorChannel == 1)
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imageWidth, imageHeight, 0, GL_RED, GL_UNSIGNED_BYTE, imageBytes);
            else
                throw std::invalid_argument("Automatic Texture type recognition failed!");
        }

        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        return textureID;
    }
}

// |=====================================================
// |---[Load Texture]------------------------------------
// |=====================================================
unsigned int GLTexture::loadTexture(const std::string& path, const char* texType,
    bool* outHasAlpha, bool* outIsFallback)
{
    // Flip on load (since OpenGL expects the origin at the bottom-left)
    stbi_set_flip_vertically_on_load(true);

    int imageWidth, imageHeight, numberOfColorChannel;
    unsigned char* imageBytes = stbi_load(path.c_str(), &imageWidth, &imageHeight, &numberOfColorChannel, 0);

    bool usingFallback = false;

    if (!imageBytes)
    {
        std::cerr << "[GLTexture] WARNING: Failed to load texture \"" << path
                    << "\" - using missing texture placeholder instead.\n";

        // Guard against a bad missingTexturePath recursing forever.
        if (path == missingTexturePath)
            throw std::runtime_error("Failed to load texture: " + path +
                                        " (the missing texture placeholder itself is invalid or missing!)");

        imageBytes = stbi_load(missingTexturePath.c_str(), &imageWidth, &imageHeight, &numberOfColorChannel, 0);
        usingFallback = true;

        if (!imageBytes)
            throw std::runtime_error("Failed to load texture: " + path +
                                        "  AND failed to load missing texture placeholder at: " + missingTexturePath);
    }

    bool hasAlpha = false;
    unsigned int texID = uploadTexture(imageBytes, imageWidth, imageHeight, numberOfColorChannel, texType, hasAlpha);

    stbi_image_free(imageBytes);

    if (outHasAlpha)   *outHasAlpha   = hasAlpha;
    if (outIsFallback) *outIsFallback = usingFallback;

	return texID;
}

// |=====================================================
// |---[Constructor]-------------------------------------
// |=====================================================
GLTexture::GLTexture(const char* image, const char* textureType, unsigned int slot)
{
    type = textureType;
    unit = slot;
    ID = loadTexture(image, textureType, &hasAlpha, &isMissingFallback);

    // Bind immediately so the texture is ready on its assigned unit
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);
}

// |=====================================================
// |---[Texture Unit]------------------------------------
// |=====================================================
void GLTexture::texUnit(Shader& shader, const char* uniform, unsigned int unit)
{
    // Tell the shader's sampler uniform which texture unit to read from
	GLuint texUni = glGetUniformLocation(shader.ID, uniform);
	shader.Use();
	glUniform1i(texUni, unit);
}

// |=====================================================
// |---[Bind, Unbind, and Delete Texture]----------------
// |=====================================================
void GLTexture::Bind()
{
	glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
}
void GLTexture::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}
void GLTexture::Delete()
{
    glDeleteTextures(1, &ID);
}
