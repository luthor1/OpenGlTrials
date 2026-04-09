#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool bloom;
uniform float exposure;

// ACES Filmic Tonemapping
vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
    // 1. Chromatic Aberration
    float strength = 0.003;
    vec3 color;
    color.r = texture(screenTexture, TexCoords + vec2(strength, 0.0)).r;
    color.g = texture(screenTexture, TexCoords).g;
    color.b = texture(screenTexture, TexCoords - vec2(strength, 0.0)).b;

    // 2. Exposure & HDR
    color *= exposure;

    // 3. Bloom (Single-pass approximation)
    vec3 bloomColor = max(color - vec3(0.8), vec3(0.0));
    color += bloomColor * 0.4;

    // 4. Tonemapping (ACES)
    vec3 result = ACESFilm(color);
    
    // 5. Vignette
    vec2 uv = TexCoords * (1.0 - TexCoords.yx);
    float vig = uv.x * uv.y * 15.0;
    vig = pow(vig, 0.15);
    result *= vig;

    // 6. Gamma correction
    result = pow(result, vec3(1.0 / 2.2));
    
    FragColor = vec4(result, 1.0);
}
