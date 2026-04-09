#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool bloom;
uniform float exposure;

void main()
{
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
    
    // Bloom (Basic Glow)
    // Professional bloom would be multi-pass Gaussian, but for a one-pass start:
    // We boost bright areas
    vec3 bloomColor = max(hdrColor - vec3(1.0), vec3(0.0));
    hdrColor += bloomColor * 0.5;

    // Tone mapping (HDR to LDR)
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    
    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));
    
    FragColor = vec4(result, 1.0);
}
