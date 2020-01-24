#ifndef RAYTRACING_STORE_BMP_HPP
#define RAYTRACING_STORE_BMP_HPP

#include <mathlib/mathlib.hpp>
#include <utils/viewport.hpp>

class StoreBMP
{
public:
    static bool Store(const char *fileName, const std::shared_ptr<Viewport>& vp);
};

#endif //RAYTRACING_STORE_BMP_HPP
