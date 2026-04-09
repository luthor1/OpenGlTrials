#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool bloom;
uniform float exposure;

uniform vec3 blackHolePos;
uniform mat4 view;
uniform mat4 projection;

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    // 1. Calculate Black Hole Screenspace Position
    vec4 bhClip = projection * view * vec4(blackHolePos, 1.0);
    vec2 bhScreen = (bhClip.xy / bhClip.w) * 0.5 + 0.5;
    
    // 2. Gravitational Lensing Distortion
    vec2 dir = TexCoords - bhScreen;
    float r = length(dir);
    
    // Einstein Ring radius
    float rs = 0.04; 
    float distortion = rs * rs / (r + 0.0001);
    
    // Check if behind camera
    if (bhClip.w < 0.0) distortion = 0.0;

    vec2 distortedCoords = TexCoords - dir * distortion;

    // 3. Event Horizon (Perfect Black)
    if (r < 0.015 && bhClip.w > 0.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 color;
    // Chromatic Aberration applied to distorted coords
    float strength = 0.002;
    color.r = texture(screenTexture, distortedCoords + vec2(strength, 0.0)).r;
    color.g = texture(screenTexture, distortedCoords).g;
    color.b = texture(screenTexture, distortedCoords - vec2(strength, 0.0)).b;

    color *= exposure;
    vec3 bloomColor = max(color - vec3(0.7), vec3(0.0));
    color += bloomColor * 0.6;
    color *= vec3(1.02, 1.0, 0.98); // Cold space tint

    vec3 result = ACESFilm(color);
    vec2 uv = TexCoords * (1.0 - TexCoords.yx);
    float vig = uv.x * uv.y * 15.0;
    vig = pow(vig, 0.15); 
    result *= vig;

    FragColor = vec4(pow(result, vec3(1.0 / 2.2)), 1.0);
}
