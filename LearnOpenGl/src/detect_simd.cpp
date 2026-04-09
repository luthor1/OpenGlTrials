#include <iostream>
#include <intrin.h>

int main() {
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    bool sse41 = (cpuInfo[2] & (1 << 19)) != 0;
    
    __cpuid(cpuInfo, 7);
    bool avx2 = (cpuInfo[1] & (1 << 5)) != 0;

    std::cout << "SSE4.1: " << (sse41 ? "Supported" : "Not supported") << std::endl;
    std::cout << "AVX2: " << (avx2 ? "Supported" : "Not supported") << std::endl;
    return 0;
}
