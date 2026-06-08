#version 460 core
out vec4 FragColor;

in vec3 color;
in vec2 TexCoord;

uniform sampler2D myTexture;
uniform vec4 uColor;

void main()
{
    FragColor = uColor;
}
