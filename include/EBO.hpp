#ifndef EBO_HPP
#define EBO_HPP

#include <glad/glad.h>

class EBO
{
public:
    unsigned int ID;
    EBO(unsigned int* indices, GLsizeiptr size);

    void Bind();
    void Unbind();
    void Delete();
};

#endif
