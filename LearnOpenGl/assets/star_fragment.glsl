#version 330 core
out vec4 FragColor;

in vec3 Color;
in float DistToCenter;

void main()
{
    // Intense core with a soft glow falloff
    vec3 core = Color * 2.0;

    // Center glow (stars near galaxy center are hotter/brighter)
    float centerGlow = 2.0 / (DistToCenter + 0.5);
    vec3 result = core + vec3(0.3, 0.5, 1.0) * centerGlow;
    
    // Slight pulse based on position (simulation of cosmic energy)
    result *= (0.9 + 0.1 * sin(gl_FragCoord.x * 0.05 + gl_FragCoord.y * 0.05));

    FragColor = vec4(result, 1.0);
}
