#version 330 core
out vec4 FragColor;
in vec3 RayDir;

// Simple hash for procedural stars
float hash(vec3 p) {
    p  = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

void main() {
    vec3 dir = normalize(RayDir);
    
    // Base space color (very dark blue/purple)
    vec3 color = vec3(0.005, 0.005, 0.015) * (1.0 - dir.y * 0.5);
    
    // Distant stars
    float starIdx = hash(floor(dir * 500.0));
    if (starIdx > 0.999) {
        float bloom = pow(hash(dir * 1234.0), 10.0) * 2.0;
        color += vec3(0.8, 0.9, 1.0) * bloom;
    }
    
    // Subtle cosmic dust / nebula
    float dust = hash(floor(dir * 50.0 + 123.0));
    color += vec3(0.1, 0.05, 0.15) * pow(dust, 20.0);

    FragColor = vec4(color, 1.0);
}
