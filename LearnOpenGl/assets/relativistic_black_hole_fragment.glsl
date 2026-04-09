#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform vec3 uCamPos;
uniform mat4 uInvView;
uniform mat4 uInvProj;
uniform float uTime;
uniform float uRs; 

const int maxSteps = 450; 
const float epsilon = 0.0001;

// Schwarzschild constants
const float diskInner = 2.6; // Innermost stable orbit is ~3Rs
const float diskOuter = 8.0;

// Physics Utility: Schwarzschild Acceleration
vec3 get_accel(vec3 p, vec3 v) {
    float r = length(p);
    if (r < uRs * 0.02) return vec3(0.0);
    vec3 L = cross(p, v);
    float h2 = dot(L, L);
    // General Relativity: a = -1.5 * Rs * (L^2 / r^5)
    return -1.5 * uRs * h2 * p / pow(r, 5.0);
}

// Visual Utility: Doppler Shift / Beaming
// Approximates blueshift (approaching) and redshift (receding)
vec3 apply_doppler(vec3 color, float v_rel) {
    // v_rel: -1 (receding) to 1 (approaching)
    float factor = pow(1.0 + v_rel * 0.8, 3.0); 
    vec3 shifted = color;
    if (v_rel > 0.0) shifted = mix(color, vec3(0.6, 0.8, 1.5), v_rel * 0.5);
    else shifted = mix(color, vec3(1.2, 0.2, 0.0), -v_rel * 0.5);
    return shifted * factor;
}

vec3 get_stars(vec3 dir) {
    float res = 100.0;
    vec3 st = dir * res;
    float noise = fract(sin(dot(floor(st), vec3(12.9898, 78.233, 45.164))) * 43758.5453);
    if (noise > 0.996) return vec3(noise * 1.8);
    return vec3(0.001, 0.001, 0.004);
}

void main() {
    vec2 uv = TexCoords * 2.0 - 1.0;
    vec4 target = uInvProj * vec4(uv, 1.0, 1.0);
    vec3 rayDir = normalize(vec3(uInvView * vec4(normalize(target.xyz), 0.0)));
    
    vec3 p = uCamPos;
    vec3 v = rayDir;
    
    vec3 diskColor = vec3(0.0);
    bool hitBH = false;
    float totalDist = 0.0;
    
    // Raymarching Geodesics (RK4 simplified internally or Euler with fine steps)
    for (int i = 0; i < maxSteps; ++i) {
        float r = length(p);
        
        // Adaptive stepping: smaller steps near Rs
        float stepSize = max(uRs * 0.03, r * 0.04);
        if (r < uRs * 4.0) stepSize = uRs * 0.012;

        // 1. Black Hole Intersection
        if (r < 1.001 * uRs) {
            hitBH = true;
            break;
        }

        // 2. Accretion Disk intersection (Physical plane at Y=0)
        // Detecting if we cross the equatorial plane
        if (p.y * (p.y + v.y * stepSize) < 0.0) {
            float t = abs(p.y / v.y);
            vec3 intersect = p + v * t;
            float ir = length(intersect);
            float ir_norm = ir / uRs;
            
            if (ir_norm > diskInner && ir_norm < diskOuter) {
                float phi = atan(intersect.z, intersect.x);
                
                // Base Disk Color (Hot orange)
                vec3 baseCol = vec3(1.0, 0.5, 0.1);
                
                // Fake Doppler: Disk rotates around Y axis
                // Tangential velocity direction is (-sin, 0, cos)
                vec3 tanVelDir = normalize(vec3(-intersect.z, 0.0, intersect.x));
                float v_rel = dot(tanVelDir, -v); // Relative velocity projection towards ray
                
                // Doppler shifted color
                vec3 finalDisk = apply_doppler(baseCol, v_rel);
                
                // Texture/Pattern: Rings and Swirls
                float pattern = sin(ir_norm * 15.0 - uTime * 3.0 + phi * 4.0) * 0.5 + 0.5;
                float noise = fract(sin(ir_norm * 456.7 + phi * 123.4) * 789.0);
                
                float alpha = smoothstep(diskInner, diskInner + 0.5, ir_norm) * smoothstep(diskOuter, diskOuter - 1.0, ir_norm);
                diskColor += finalDisk * (0.6 + 0.4 * pattern + 0.2 * noise) * alpha * 2.0;
                
                // Transmission: Some rays pass through, some are absorbed
                if (diskColor.r > 1.0) break; 
            }
        }

        // 3. Update Trajectory (Schwarzschild Deflection)
        // Using a high-order-like update for smoothness
        vec3 accel = get_accel(p, v);
        v += accel * stepSize;
        v = normalize(v); // Keep light speed constant
        p += v * stepSize;
        
        totalDist += stepSize;
        if (totalDist > uRs * 1000.0) break;
    }

    vec3 skyColor = hitBH ? vec3(0.0) : get_stars(v);
    
    // Combine disk and background
    vec3 finalColor = diskColor + skyColor * (1.0 - clamp(length(diskColor), 0.0, 1.0));
    
    // ACES Tone Mapping for Cinematic Look
    finalColor *= 0.7; 
    finalColor = (finalColor * (2.51 * finalColor + 0.03)) / (finalColor * (2.43 * finalColor + 0.59) + 0.14);
    
    FragColor = vec4(finalColor, 1.0);
}
