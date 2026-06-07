#include "Renderer.hpp"
#include <GL/gl.h>
#include <functional>

float rectangleVertices[] =
{
        // positions
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
};

unsigned int rectangleIndices[] =
{
        0, 1, 2,
        2, 3, 0
};

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::init()
{
    // ===[Generates shader object using vertex and fragment shaders .glsl files]===
    shader = new Shader("../shader_files/VertexShader.glsl", "../shader_files/FragmentShader.glsl");

   	// Generates Vertex Array Object and binds it
	vao = new VAO();
	vao->Bind();

	// Generates Vertex Buffer Object and links it to vertices
	vbo = new VBO(rectangleVertices, sizeof(rectangleVertices));
	// Generates Element Buffer Object and links it to indices
	ebo = new EBO(rectangleIndices, sizeof(rectangleIndices));

	// Links VBO attributes such as coordinates and colors to VAO
	vao->LinkVBO(*vbo, 0);
	// Unbind all to prevent accidentally modifying them
	vao->Unbind();
	vbo->Unbind();
	ebo->Unbind();

	// Set the scale uniform once after shader is ready
    shader->Use();
    GLuint uniID = glGetUniformLocation(shader->ID, "scale");
    glUniform1f(uniID, 1.0f);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Renderer::drawUXXPanel(Rect rect, Color color)
{
    shader->Use();
    vao->Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    vao->Unbind();
}

void Renderer::drawUXXButton(Rect rect, Color color, Style style)
{

}

void Renderer::drawUXXSlider(Rect rect, Color color, Style style)
{

}

void Renderer::drawUXXSwitch(Rect rect, Color color, Style style)
{

}

void Renderer::blowup()
{

}
