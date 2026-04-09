#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 RayDir;

uniform mat4 invView;
uniform mat4 invProjection;

void main() {
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0, 1.0); // Depth = 1.0 (behind everything)
    
    // Compute view ray direction in world space
    vec4 ray_eye = invProjection * vec4(aPos, -1.0, 1.0);
    ray_eye = vec4(ray_eye.xy, -1.0, 0.0);
    RayDir = (invView * ray_eye).xyz;
}
