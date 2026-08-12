#include "internal/Texture.hpp"

// |=====================================================
// |---[Load Texture]------------------------------------
// |=====================================================
unsigned int GLTexture::loadTexture(const std::string& path, const char* texType)
{
    int imgWidth, imgHeight, numColorChannel;
    // Flip on load (since OpenGL expects the origin at the bottom-left)
    stbi_set_flip_vertically_on_load(true);
    unsigned char* imageBytes = stbi_load(path.c_str(), &imgWidth, &imgHeight, &numColorChannel, 0);
    if (!imageBytes)
        throw std::runtime_error("Failed to load texture: " + path);

    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // ===[Pick the correct internal/source format based on texture type and channel count]===
    if (strcmp(texType, "normal") == 0)
	{
		if (numColorChannel == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBytes);
		else if (numColorChannel == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBytes);
		else
			throw std::invalid_argument("Normal Texture type recognition failed!");
	}
	else if (strcmp(texType, "displacement") == 0)
	{
		if (numColorChannel == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBytes);
		else if (numColorChannel == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBytes);
		else if (numColorChannel == 1)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imgWidth, imgHeight, 0, GL_RED, GL_UNSIGNED_BYTE, imageBytes);
		else
			throw std::invalid_argument("Displacement Texture type recognition failed!");
	}
	else if (strcmp(texType, "specular") ==0)
	{
		if (numColorChannel == 4)
		        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBytes);
		else if (numColorChannel == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBytes);
		else if (numColorChannel == 1)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imgWidth, imgHeight, 0, GL_RED, GL_UNSIGNED_BYTE, imageBytes);
		else
			throw std::invalid_argument("Specular Texture type recognition failed!");
	}
	else
	{
		if (numColorChannel == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBytes);
		else if (numColorChannel == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageBytes);
		else if (numColorChannel == 1)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imgWidth, imgHeight, 0, GL_RED, GL_UNSIGNED_BYTE, imageBytes);
		else
			throw std::invalid_argument("Automatic Texture type recognition failed!");
	}

    glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(imageBytes);
	glBindTexture(GL_TEXTURE_2D, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	return texID;
}

// |=====================================================
// |---[Constructor]-------------------------------------
// |=====================================================
GLTexture::GLTexture(const char* image, const char* texType, unsigned int slot)
{
    type = texType;
    unit = slot;
    ID = loadTexture(image, texType);

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
