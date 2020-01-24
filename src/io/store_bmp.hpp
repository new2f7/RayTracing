#ifndef RAYTRACING_STORE_BMP_HPP
#define RAYTRACING_STORE_BMP_HPP

#include <mathlib/mathlib.hpp>

class StoreBMP
{
public:
    static bool Store(const char *fileName, float3* pixels, unsigned int width, unsigned int height);
};

#endif //RAYTRACING_STORE_BMP_HPP
