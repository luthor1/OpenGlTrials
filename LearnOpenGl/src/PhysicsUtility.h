#pragma once
#include <immintrin.h>

namespace Physics {
    // Basic SIMD Vector Math Helpers
    
    // Updates velocity based on acceleration: v = v + a * dt
    inline void UpdateVelocity(__m256& vx, __m256& vy, __m256& vz, 
                               __m256 ax, __m256 ay, __m256 az, 
                               __m256 dt) {
        vx = _mm256_add_ps(vx, _mm256_mul_ps(ax, dt));
        vy = _mm256_add_ps(vy, _mm256_mul_ps(ay, dt));
        vz = _mm256_add_ps(vz, _mm256_mul_ps(az, dt));
    }

    // Updates position based on velocity: p = p + v * dt
    inline void UpdatePosition(__m256& px, __m256& py, __m256& pz, 
                                __m256 vx, __m256 vy, __m256 vz, 
                                __m256 dt) {
        px = _mm256_add_ps(px, _mm256_mul_ps(vx, dt));
        py = _mm256_add_ps(py, _mm256_mul_ps(vy, dt));
        pz = _mm256_add_ps(pz, _mm256_mul_ps(vz, dt));
    }

    // Applies friction/damping: v = v * friction
    inline void ApplyFriction(__m256& vx, __m256& vy, __m256& vz, __m256 friction) {
        vx = _mm256_mul_ps(vx, friction);
        vy = _mm256_mul_ps(vy, friction);
        vz = _mm256_mul_ps(vz, friction);
    }
}
