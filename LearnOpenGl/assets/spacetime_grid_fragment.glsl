#version 330 core
out vec4 FragColor;
in float vBending;

void main() {
    // Dynamic color based on bending intensity
    float intensity = clamp(abs(vBending) * 0.2, 0.0, 1.0);
    vec3 baseColor = vec3(0.0, 0.5, 1.0); // Neon Blue
    vec3 hotColor = vec3(1.0, 0.2, 0.5); // Pinkish red for deep wells
    
    vec3 finalColor = mix(baseColor, hotColor, intensity);
    
    // Add a grid glow effect
    FragColor = vec4(finalColor, 0.8);
}
