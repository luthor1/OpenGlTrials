#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 uColor; // Constant color for the whole mesh

void main()
{
    FragPos = aPos;
    Normal = aNormal;
    Color = uColor;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
