#version 460 core
out vec4 FragColor;

in vec3 color;
in vec2 texCoord;

uniform sampler2D tex0;
uniform vec4 uColor;
uniform bool uUseTexture;
uniform float uDarken;

void main()
{
    if (uUseTexture)
    {
        vec4 texColor = texture(tex0, texCoord);
        // Darken the image itself but leave its own alpha untouched so
        // transparent or partially-transparent pixels don't get affected
        FragColor = vec4(texColor.rgb * uDarken, texColor.a);
    }
    else
        // If there are no images then arkening is expressed as a distinct flat color chosen by the caller
        FragColor = uColor;
}
