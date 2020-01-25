#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "mathlib/mathlib.hpp"

struct Viewport
{
public:
    Viewport(size_t width, size_t height)
        : width(width), height(height)
    {
        pixels = new float[width * height * 4]; // RGBA
    }

    ~Viewport() { if (pixels) delete[] pixels; }

    unsigned int width, height;
    float* pixels;

};

#endif // VIEWPORT_HPP
