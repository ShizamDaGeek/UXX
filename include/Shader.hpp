#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/glad.h>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Getting the file contents
std::string get_file_contents(const char* filename);

class Shader
{
public:
    // The program ID
    unsigned int ID;

    // constructor reads and builds the shader
    Shader(const char* vertexFile, const char* fragmentFile);

    void Use();
    void Delete();

    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

private:
    std::string vertexFilePath;
    std::string fragmentFilePath;
	void compileErrors(unsigned int shader, const char* type, const char* filePath);
};
#endif
