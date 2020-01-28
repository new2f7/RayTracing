#ifndef OCL_HELPER_HPP
#define OCL_HELPER_HPP

#include "scene/scene.hpp"
#include "noma/ocl/helper.hpp"
#include <CL/cl2.hpp>
#include <memory>

enum class RenderKernelArgument_t : unsigned int
{
    BUFFER_OUT,
    BUFFER_SCENE,
    BUFFER_NODE,
    BUFFER_MATERIAL,
    WIDTH,
    HEIGHT,
    CAM_ORIGIN,
    CAM_FRONT,
    CAM_UP,
    FRAME_COUNT,
    TEXTURE0,
};


class OCLHelper
{
public:
    OCLHelper(const std::string config_file);


    void CreateProgramFromFile(const std::string kernel_file, const std::string kernel_name);

    void SetArgument(RenderKernelArgument_t argIndex, void* data, size_t size);

    const cl::Kernel& GetKernel() const { return m_Kernel; }

    void ReadBuffer(const cl::Buffer& buffer, void* ptr, size_t size) const;

    cl_ulong RunKernelTimed(size_t work_items) const;

private:
    noma::ocl::helper m_ocl_helper;
    ocl_config m_ocl_config;
    cl::Kernel  m_Kernel;
    cl::Program m_Program;

};

#endif // OCL_HELPER_HPP
