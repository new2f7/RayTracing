#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP

#include "scene/camera.hpp"
#include "scene/scene.hpp"
#include "ocl_helper/ocl_helper.hpp"
#include "utils/viewport.hpp"
#include "noma/ocl/helper.hpp"
#include <memory>
#include <ctime>

#define BVH_INTERSECTION

class Render
{
public:
    void         Init(std::string config_file, size_t width, size_t height);
    cl_ulong     RenderFrame();
    void         Shutdown();

    double       GetCurtime()        const;
    unsigned int GetGlobalWorkSize() const;

    std::shared_ptr<OCLHelper>  GetOCLHelper()  const;

private:
    void SetupBuffers();

private:
    // OCLHelper
    std::shared_ptr<OCLHelper>  m_OCLHelper;
    // Scene
    std::shared_ptr<Camera>     m_Camera;
    std::shared_ptr<Scene>      m_Scene;
    std::shared_ptr<Viewport>   m_Viewport;
    // Buffers
    cl::Buffer m_OutputBuffer;
    cl::Image2D m_Texture0;

};

extern Render* render;

#endif // RAYTRACER_HPP
