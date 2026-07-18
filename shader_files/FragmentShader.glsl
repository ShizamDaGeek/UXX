#version 460 core
out vec4 FragColor;

in vec3 color;
in vec2 texCoord;

uniform sampler2D tex0;
uniform vec4 uColor;
uniform bool uUseTexture;

void main()
{
    if (uUseTexture)
        FragColor = texture(tex0, texCoord) * uColor;
    else
        FragColor = uColor;
}
