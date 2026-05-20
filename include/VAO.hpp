#ifndef VAO_HPP
#define VAO_HPP

#include <glad/glad.h>
#include "VBO.hpp"

class VAO
{
public:
    unsigned int ID;
    VAO();

    void LinkVBO(VBO& VBO, unsigned int layout);
    void Bind();
    void Unbind();
    void Delete();
};

#endif
