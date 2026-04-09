#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform vec3 uCamPos;
uniform mat4 uInvView;
uniform mat4 uInvProj;
uniform float uTime;

const float G = 1.0;
const float M = 1.0;
const float Rs = 2.0 * G * M;
const float stepSize = 0.15;
const int maxSteps = 200; // Adjusted for 3.3 compliance and performance

const float diskInner = 3.0 * Rs;
const float diskOuter = 8.0 * Rs;

vec3 get_accel(vec3 p, vec3 v) {
    float r = length(p);
    if (r < 0.001) return vec3(0.0);
    vec3 L = cross(p, v);
    float h2 = dot(L, L);
    return -1.5 * Rs * h2 * p / pow(r, 5.0);
}

vec3 get_stars(vec3 dir) {
    float res = 50.0;
    vec3 st = dir * res;
    float noise = fract(sin(dot(floor(st), vec3(12.9898, 78.233, 45.164))) * 43758.5453);
    if (noise > 0.995) return vec3(noise);
    return vec3(0.01, 0.01, 0.02);
}

vec3 get_disk_color(float r, float phi) {
    float t = (r - diskInner) / (diskOuter - diskInner);
    vec3 innerColor = vec3(1.0, 0.5, 0.1);
    vec3 outerColor = vec3(0.1, 0.3, 1.0);
    vec3 col = mix(innerColor, outerColor, t);
    float swirl = sin(phi * 8.0 + uTime * 2.0 + r * 0.5);
    col *= (0.8 + 0.2 * swirl);
    float alpha = smoothstep(0.0, 0.1, t) * smoothstep(1.0, 0.9, t);
    return col * alpha;
}

void main() {
    vec2 uv = TexCoords * 2.0 - 1.0;
    vec4 target = uInvProj * vec4(uv, 1.0, 1.0);
    vec3 rayDir = normalize(vec3(uInvView * vec4(normalize(target.xyz), 0.0)));
    
    vec3 p = uCamPos;
    vec3 v = rayDir;
    vec3 finalColor = vec3(0.0);
    bool hitBH = false;
    float totalDist = 0.0;

    for (int i = 0; i < maxSteps; ++i) {
        float r = length(p);
        if (r < 1.01 * Rs) {
            hitBH = true;
            break;
        }

        // Disk intersection
        if (p.y * (p.y + v.y * stepSize) < 0.0) {
            float t = -p.y / v.y;
            vec3 intersect = p + v * t;
            float ir = length(intersect);
            if (ir > diskInner && ir < diskOuter) {
                float phi = atan(intersect.z, intersect.x);
                finalColor += get_disk_color(ir, phi) * 0.7;
            }
        }

        // RK4 Integration
        vec3 k1_v = get_accel(p, v) * stepSize;
        vec3 k1_p = v * stepSize;
        vec3 k2_v = get_accel(p + k1_p * 0.5, v + k1_v * 0.5) * stepSize;
        vec3 k2_p = (v + k1_v * 0.5) * stepSize;
        vec3 k3_v = get_accel(p + k2_p * 0.5, v + k2_v * 0.5) * stepSize;
        vec3 k3_p = (v + k2_v * 0.5) * stepSize;
        vec3 k4_v = get_accel(p + k3_p, v + k3_v) * stepSize;
        vec3 k4_p = (v + k3_v) * stepSize;

        v += (k1_v + 2.0*k2_v + 2.0*k3_v + k4_v) / 6.0;
        p += (k1_p + 2.0*k2_p + 2.0*k3_p + k4_p) / 6.0;
        
        totalDist += stepSize;
        if (totalDist > 100.0) break;
    }

    if (!hitBH) {
        finalColor += get_stars(normalize(v));
    } else {
        finalColor = vec3(0.0);
    }

    FragColor = vec4(finalColor, 1.0);
}
