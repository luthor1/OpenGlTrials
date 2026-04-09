#version 330 core

out vec4 FragColor;
in vec2 TexCoords;

layout(std140, binding = 1) uniform Camera {
    vec3 pos;
    float tanHalfFov;
    vec3 right;
    float aspect;
    vec3 up;
    int moving;
    vec3 forward;
    float uTime;
} cam;

layout(std140, binding = 2) uniform Disk {
    float r1;
    float r2;
    float num;
    float thickness;
} diskData;

layout(std140, binding = 3) uniform Objects {
    int numObjects;
    vec4 posRadius[16]; // xyz: pos, w: radius
    vec4 colorMass[16]; // rgb: color, a: mass
} objs;

const float c = 299792458.0;
const float G = 6.67430e-11;
const float SagA_mass = 8.54e36;
const float Rs = 2.0 * G * SagA_mass / (c * c);

// Cinematic Constants
const float diskInner = 2.6; 
const float diskOuter = 8.0;

vec3 get_accel(vec3 p, vec3 v) {
    float r = length(p);
    if (r < Rs * 0.02) return vec3(0.0);
    vec3 L = cross(p, v);
    float h2 = dot(L, L);
    return -1.5 * Rs * h2 * p / pow(r, 5.0);
}

vec3 apply_doppler(vec3 color, float v_rel) {
    float factor = pow(1.0 + v_rel * 0.8, 3.0); 
    vec3 shifted = color;
    if (v_rel > 0.0) shifted = mix(color, vec3(0.6, 0.8, 1.5), v_rel * 0.5);
    else shifted = mix(color, vec3(1.2, 0.2, 0.0), -v_rel * 0.5);
    return shifted * factor;
}

vec3 get_stars(vec3 dir) {
    float noise = fract(sin(dot(floor(dir * 100.0), vec3(12.9898, 78.233, 45.164))) * 43758.5453);
    if (noise > 0.996) return vec3(noise * 1.5);
    return vec3(0.005, 0.005, 0.015);
}

void main() {
    vec2 uv = TexCoords * 2.0 - 1.0;
    uv.x *= cam.aspect;

    vec3 rayDir = normalize(cam.forward + cam.right * uv.x * cam.tanHalfFov + cam.up * uv.y * cam.tanHalfFov);

    vec3 p = cam.pos;
    vec3 v = rayDir;
    
    vec3 diskColor = vec3(0.0);
    bool hitBH = false;
    float totalDist = 0.0;
    int maxSteps = cam.moving == 1 ? 120 : 350;

    for (int i = 0; i < maxSteps; ++i) {
        float r = length(p);
        float stepSize = max(Rs * 0.05, r * 0.05);
        if (r < Rs * 4.0) stepSize = Rs * 0.02;

        if (r < 1.002 * Rs) {
            hitBH = true;
            break;
        }

        // Accretion Disk Interaction
        if (p.y * (p.y + v.y * stepSize) < 0.0) {
            float t = abs(p.y / v.y);
            vec3 intersect = p + v * t;
            float ir = length(intersect);
            float ir_norm = ir / Rs;
            
            if (ir_norm > diskInner && ir_norm < diskOuter) {
                float phi = atan(intersect.z, intersect.x);
                vec3 baseCol = vec3(1.0, 0.5, 0.1);
                
                vec3 tanVelDir = normalize(vec3(-intersect.z, 0.0, intersect.x));
                float v_rel = dot(tanVelDir, -v);
                vec3 finalDisk = apply_doppler(baseCol, v_rel);
                
                float pattern = sin(ir_norm * 12.0 - cam.uTime * 3.0 + phi * 4.0) * 0.5 + 0.5;
                float alpha = smoothstep(diskInner, diskInner + 0.5, ir_norm) * smoothstep(diskOuter, diskOuter - 1.0, ir_norm);
                diskColor += finalDisk * (0.8 + 0.2 * pattern) * alpha * 1.5;
                
                if (length(diskColor) > 1.2) break; 
            }
        }

        vec3 accel = get_accel(p, v);
        v += accel * stepSize;
        v = normalize(v); 
        p += v * stepSize;
        
        totalDist += stepSize;
        if (totalDist > 1e13) break;
    }

    vec3 skyColor = hitBH ? vec3(0.0) : get_stars(v);
    vec3 finalColor = diskColor + skyColor * (1.0 - clamp(length(diskColor), 0.0, 1.0));
    
    finalColor = finalColor / (finalColor + vec3(1.0));
    FragColor = vec4(finalColor, 1.0);
}
