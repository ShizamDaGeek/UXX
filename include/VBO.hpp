#ifndef VBO_HPP
#define VBO_HPP

#include <glad/glad.h>

class VBO
{
public:
    unsigned int ID;
    VBO(GLfloat* vertices, GLsizeiptr size);

    void Bind();
    void Unbind();
    void Delete();
};

#endif
