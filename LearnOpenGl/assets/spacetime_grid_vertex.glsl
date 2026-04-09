#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

// Support up to 16 significant mass points
uniform vec4 u_Masses[16]; // xyz: pos, w: schwarzschild radius (rs)
uniform int u_MassCount;

out float vBending;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    float totalBending = 0.0;

    for (int i = 0; i < u_MassCount; ++i) {
        vec3 mPos = u_Masses[i].xyz;
        float rs = u_Masses[i].w;
        
        float r = distance(worldPos.xz, mPos.xz);
        
        // Flamm Paraboloid implementation: y = 2 * sqrt(rs * (r - rs))
        // We use it as a displacement from the flat plane (y = 0)
        // If r < rs, it's inside the event horizon, we cap the depth.
        if (r > rs) {
            float h = 2.0 * sqrt(rs * (r - rs));
            // We want a 'well' effect, so we adjust the base
            totalBending -= (4.0 * sqrt(rs * 10.0) - h); 
        } else {
            totalBending -= 4.0 * sqrt(rs * 10.0);
        }
    }

    worldPos.y += totalBending;
    vBending = totalBending;
    gl_Position = projection * view * worldPos;
}
