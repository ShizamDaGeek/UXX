#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

out vec3 color;

uniform vec2 uPosition;
uniform vec2 uSize;
uniform float uRotation;

void main()
{
    // Scale
    vec2 scaled = aPos.xy * uSize * 2.0;

    // Rotation
    float cosRot = cos(uRotation);
    float sinRot = sin(uRotation);
    vec2 rotated = vec2(
            scaled.x * cosRot - scaled.y * sinRot,
            scaled.x * sinRot + scaled.y * cosRot);

    vec2 finalPos = rotated + uPosition;
    gl_Position = vec4(finalPos, 0.0, 1.0);
    color = aColor;
}
