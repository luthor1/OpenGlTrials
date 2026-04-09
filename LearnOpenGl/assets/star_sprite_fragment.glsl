#version 330 core
out vec4 FragColor;
in vec3 StarColor;

void main() {
    // Generate radial gradient for soft star look
    vec2 circPoint = gl_PointCoord * 2.0 - 1.0;
    float dist = dot(circPoint, circPoint);
    
    if (dist > 1.0) discard;
    
    // Smooth radial falloff (Gaussian-ish)
    float alpha = exp(-dist * 4.0);
    
    // Core glow
    float core = exp(-dist * 20.0);
    vec3 color = StarColor + core * 2.0;
    
    FragColor = vec4(color, alpha);
}
