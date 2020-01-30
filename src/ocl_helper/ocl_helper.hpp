#ifndef OCL_HELPER_HPP
#define OCL_HELPER_HPP

#include "scene/scene.hpp"
#include "noma/ocl/helper.hpp"
#include <CL/cl.hpp>
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

    const cl::Context& GetContext() const { return m_ocl_helper->context(); }
    std::shared_ptr<noma::ocl::helper> GetOCLHelper() const { return m_ocl_helper; }

    void CreateProgramFromFile(const std::string kernel_file, const std::string kernel_name);

    void SetArgument(RenderKernelArgument_t argIndex, void* data, size_t size);

    cl_ulong RunKernelTimed(size_t work_items);

    void ReadBuffer(const cl::Buffer& buffer, void* ptr, size_t size) const;

private:
    std::shared_ptr<noma::ocl::helper> m_ocl_helper;
    std::shared_ptr<noma::ocl::config> m_ocl_config;
    cl::Kernel  m_Kernel;
    cl::Program m_Program;

};

#endif // OCL_HELPER_HPP
