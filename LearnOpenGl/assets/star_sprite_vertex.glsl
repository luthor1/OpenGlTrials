#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 view;
uniform mat4 projection;
uniform float scale;

out vec3 StarColor;

void main() {
    StarColor = aColor;
    vec4 viewPos = view * vec4(aPos, 1.0);
    gl_Position = projection * viewPos;
    
    // Dynamic Point Size based on distance to camera
    float dist = length(viewPos.xyz);
    gl_PointSize = (scale * 1000.0) / dist;
}
