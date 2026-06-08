#include "Renderer.hpp"
#include <GL/gl.h>

float rectangleVertices[] =
{
    // positions          // Color
    -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f,
    0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f,
    0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f
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
	vao->LinkAttrib(*vbo, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
	vao->LinkAttrib(*vbo, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	// Unbind all to prevent accidentally modifying them
	vao->Unbind();
	vbo->Unbind();
	ebo->Unbind();

	// Set the scale uniform once after shader is ready
    shader->Use();
    GLuint scale = glGetUniformLocation(shader->ID, "scale");
    glUniform1f(scale, 1.0f);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Renderer::BeginUXXPanel(Rect rect, Color color)
{
    panelOpen = true;
    shader->Use();

    float SCREEN_WIDTH = 1920.0f;
    float SCREEN_HEIGHT = 1080.0f;

    // ===[Color]===
    GLuint colorLoc = glGetUniformLocation(shader->ID, "uColor");
    glUniform4f(colorLoc, color.r, color.g, color.b, color.a);

    // ===[Size]===
    GLuint sizeLoc = glGetUniformLocation(shader->ID, "uSize");
    glUniform2f(sizeLoc, (rect.width / SCREEN_WIDTH) * 2.0f, (rect.height / SCREEN_HEIGHT) * 2.0f);

    // ===[Position]===
    GLuint positionLoc = glGetUniformLocation(shader->ID, "uPosition");
    glUniform2f(positionLoc, ((rect.xPos + rect.width * 0.5f) / SCREEN_WIDTH) * 2.0f - 1.0f,
        1.0f - ((rect.yPos + rect.height * 0.5f) / SCREEN_HEIGHT) * 2.0f);

    // ===[Rotation]===
    GLuint rotationLoc = glGetUniformLocation(shader->ID, "uRotation");
    glUniform1f(rotationLoc, rect.rotation);

    vao->Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    vao->Unbind();
}

void Renderer::EndUXXPanel()
{
    panelOpen = false;
}

void Renderer::drawUXXButton(Rect rect, Color color)
{
    if (panelOpen)
    {
        shader->Use();

        float SCREEN_WIDTH = 1920.0f;
        float SCREEN_HEIGHT = 1080.0f;

        // ===[Color]===
        GLuint colorLoc = glGetUniformLocation(shader->ID, "uColor");
        glUniform4f(colorLoc, color.r, color.g, color.b, color.a);

        // ===[Size]===
        GLuint sizeLoc = glGetUniformLocation(shader->ID, "uSize");
        glUniform2f(sizeLoc, (rect.width / SCREEN_WIDTH) * 2.0f, (rect.height / SCREEN_HEIGHT) * 2.0f);

        // ===[Position]===
        GLuint positionLoc = glGetUniformLocation(shader->ID, "uPosition");
        glUniform2f(positionLoc, ((rect.xPos + rect.width * 0.5f) / SCREEN_WIDTH) * 2.0f - 1.0f,
            1.0f - ((rect.yPos + rect.height * 0.5f) / SCREEN_HEIGHT) * 2.0f);

        // ===[Rotation]===
        GLuint rotationLoc = glGetUniformLocation(shader->ID, "uRotation");
        glUniform1f(rotationLoc, rect.rotation);

        vao->Bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        vao->Unbind();
    }
    else
    {
        return;
    }
}

void Renderer::drawUXXSlider(Rect rect, Color color)
{

}

void Renderer::drawUXXSwitch(Rect rect, Color color)
{

}

void Renderer::blowup()
{

}
