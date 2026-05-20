#include "Texture.hpp"

unsigned int GLTexture::loadTexture(const std::string& path, const char* texType)
{
        int imgWidth, imgHeight, numColorChannel;
        unsigned char* bytes = stbi_load(path.c_str(), &imgWidth, &imgHeight, &numColorChannel, 0);
        if (!bytes)
                throw std::runtime_error("Failed to load texture: " + path);

        unsigned int texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

       	if (strcmp(texType, "normal") == 0)
	{
		if (numColorChannel == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
		else if (numColorChannel == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
		else
			throw std::invalid_argument("Normal Texture type recognition failed!");
	}
	else if (strcmp(texType, "displacement") == 0)
	{
		if (numColorChannel == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
		else if (numColorChannel == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
		else if (numColorChannel == 1)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imgWidth, imgHeight, 0, GL_RED, GL_UNSIGNED_BYTE, bytes);
		else
			throw std::invalid_argument("Displacement Texture type recognition failed!");
	}
	else if (strcmp(texType, "specular") ==0)
	{
		if (numColorChannel == 4)
		        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
		else if (numColorChannel == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
		else if (numColorChannel == 1)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imgWidth, imgHeight, 0, GL_RED, GL_UNSIGNED_BYTE, bytes);
		else
			throw std::invalid_argument("Specular Texture type recognition failed!");
	}
	else
	{
		if (numColorChannel == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
		else if (numColorChannel == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgWidth, imgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
		else if (numColorChannel == 1)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, imgWidth, imgHeight, 0, GL_RED, GL_UNSIGNED_BYTE, bytes);
		else
			throw std::invalid_argument("Automatic Texture type recognition failed!");
	}

       	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(bytes);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}
GLTexture::GLTexture(const char* image, const char* texType, unsigned int slot)
{
        type = texType;
        unit = slot;
        ID = loadTexture(image, texType);

        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, ID);
}
void GLTexture::texUnit(Shader& shader, const char* uniform, unsigned int unit)
{
	GLuint texUni = glGetUniformLocation(shader.ID, uniform);

	shader.Use();

	// Sets the value of the uniform
	glUniform1i(texUni, unit);
}

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
