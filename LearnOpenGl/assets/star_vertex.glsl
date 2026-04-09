#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aInstancePos;
layout (location = 3) in vec3 aInstanceColor;

out vec3 Color;
out float DistToCenter;

uniform mat4 view;
uniform mat4 projection;
uniform float scale;

void main()
{
    Color = aInstanceColor;
    DistToCenter = length(aInstancePos);
    
    // Scale stars for distant visibility
    float dynamicScale = scale * (1.1 + DistToCenter * 0.05);
    
    vec3 worldPos = aInstancePos + aPos * dynamicScale;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}
