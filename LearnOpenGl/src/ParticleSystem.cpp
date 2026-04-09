#include "ParticleSystem.h"
#include <immintrin.h>
#include <malloc.h>
#include <random>
#include <algorithm>

ParticleSystem::ParticleSystem(int maxParticles) : m_MaxParticles(maxParticles) {
    // Round up to multiple of 8 for AVX (8 floats per register)
    int count = (maxParticles + 7) & ~7;
    
    // Allocate aligned memory (32 bytes for AVX)
    m_PosX = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_PosY = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_VelX = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_VelY = (float*)_aligned_malloc(count * sizeof(float), 32);

    Reset();
}

ParticleSystem::~ParticleSystem() {
    _aligned_free(m_PosX);
    _aligned_free(m_PosY);
    _aligned_free(m_VelX);
    _aligned_free(m_VelY);
}

void ParticleSystem::Reset() {
    std::default_random_engine gen;
    std::uniform_real_distribution<float> distPos(-0.5f, 0.5f);
    std::uniform_real_distribution<float> distVel(-1.0f, 1.0f);

    for (int i = 0; i < m_MaxParticles; ++i) {
        m_PosX[i] = distPos(gen);
        m_PosY[i] = distPos(gen);
        m_VelX[i] = distVel(gen);
        m_VelY[i] = distVel(gen);
    }
}

void ParticleSystem::Update(float dt) {
    if (UseSIMD) {
        UpdateSIMD(dt);
    } else {
        UpdateScalar(dt);
    }
}

void ParticleSystem::UpdateScalar(float dt) {
    for (int i = 0; i < m_MaxParticles; ++i) {
        // Physics
        m_VelY[i] -= Gravity * dt;
        m_VelX[i] *= Friction;
        m_VelY[i] *= Friction;

        m_PosX[i] += m_VelX[i] * dt;
        m_PosY[i] += m_VelY[i] * dt;

        // Boundaries
        if (m_PosY[i] < -1.0f) {
            m_PosY[i] = -1.0f;
            m_VelY[i] *= -Bounciness;
        }
        if (m_PosX[i] < -1.0f || m_PosX[i] > 1.0f) {
            m_VelX[i] *= -Bounciness;
            if (m_PosX[i] < -1.0f) m_PosX[i] = -1.0f;
            if (m_PosX[i] > 1.0f) m_PosX[i] = 1.0f;
        }
    }
}

void ParticleSystem::UpdateSIMD(float dt) {
    // AVX2 handles 8 floats at a time
    __m256 dtV = _mm256_set1_ps(dt);
    __m256 gravityV = _mm256_set1_ps(Gravity);
    __m256 frictionV = _mm256_set1_ps(Friction);
    __m256 bouncinessV = _mm256_set1_ps(Bounciness);
    __m256 negOneV = _mm256_set1_ps(-1.0f);
    __m256 oneV = _mm256_set1_ps(1.0f);
    __m256 zeroV = _mm256_setzero_ps();

    for (int i = 0; i < m_MaxParticles; i += 8) {
        // Load data
        __m256 px = _mm256_load_ps(&m_PosX[i]);
        __m256 py = _mm256_load_ps(&m_PosY[i]);
        __m256 vx = _mm256_load_ps(&m_VelX[i]);
        __m256 vy = _mm256_load_ps(&m_VelY[i]);

        // Update velocity (v = v + a*dt)
        // Gravity only affects Y
        __m256 gravityStep = _mm256_mul_ps(gravityV, dtV);
        vy = _mm256_sub_ps(vy, gravityStep);
        
        // Apply friction
        vx = _mm256_mul_ps(vx, frictionV);
        vy = _mm256_mul_ps(vy, frictionV);

        // Update position (p = p + v*dt)
        px = _mm256_add_ps(px, _mm256_mul_ps(vx, dtV));
        py = _mm256_add_ps(py, _mm256_mul_ps(vy, dtV));

        // Boundary checks (simulated with masks/selects for branchless)
        // If py < -1.0
        __m256 bottomMask = _mm256_cmp_ps(py, negOneV, _CMP_LT_OQ);
        py = _mm256_blendv_ps(py, negOneV, bottomMask);
        __m256 reflectedVy = _mm256_mul_ps(vy, _mm256_set1_ps(-Bounciness));
        vy = _mm256_blendv_ps(vy, reflectedVy, bottomMask);

        // X boundaries
        __m256 leftMask = _mm256_cmp_ps(px, negOneV, _CMP_LT_OQ);
        __m256 rightMask = _mm256_cmp_ps(px, oneV, _CMP_GT_OQ);
        __m256 xMask = _mm256_or_ps(leftMask, rightMask);
        
        px = _mm256_blendv_ps(px, negOneV, leftMask);
        px = _mm256_blendv_ps(px, oneV, rightMask);
        
        __m256 reflectedVx = _mm256_mul_ps(vx, _mm256_set1_ps(-Bounciness));
        vx = _mm256_blendv_ps(vx, reflectedVx, xMask);

        // Store data
        _mm256_store_ps(&m_PosX[i], px);
        _mm256_store_ps(&m_PosY[i], py);
        _mm256_store_ps(&m_VelX[i], vx);
        _mm256_store_ps(&m_VelY[i], vy);
    }
}
