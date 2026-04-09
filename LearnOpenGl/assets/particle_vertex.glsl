#version 330 core
layout (location = 0) in vec2 aPos;       // Base quad vertices
layout (location = 1) in vec2 aOffset;    // Per-instance position

uniform mat4 projection;
uniform float particleSize;

void main()
{
    gl_Position = projection * vec4(aPos * particleSize + aOffset, 0.0, 1.0);
}
