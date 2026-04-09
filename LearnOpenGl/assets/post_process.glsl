#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool bloom;
uniform float exposure;

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec3 color;
    // Chromatic Aberration
    float strength = 0.002;
    color.r = texture(screenTexture, TexCoords + vec2(strength, 0.0)).r;
    color.g = texture(screenTexture, TexCoords).g;
    color.b = texture(screenTexture, TexCoords - vec2(strength, 0.0)).b;

    // Enhance contrast and exposure
    color *= exposure;

    // Soft Bloom
    vec3 bloomColor = max(color - vec3(0.7), vec3(0.0));
    color += bloomColor * 0.5;

    // Cinematic Grading (Warm tint)
    color *= vec3(1.05, 1.0, 0.95);

    // ACES Tonemapping
    vec3 result = ACESFilm(color);

    // Vignette
    vec2 uv = TexCoords * (1.0 - TexCoords.yx);
    float vig = uv.x * uv.y * 15.0;
    vig = pow(vig, 0.2); 
    result *= vig;

    // Final Gamma
    FragColor = vec4(pow(result, vec3(1.0 / 2.2)), 1.0);
}
