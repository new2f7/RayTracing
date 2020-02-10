#include "ocl_helper.hpp"
#include "utils/cl_exception.hpp"
#include "renderers/render.hpp"
#include "noma/bmt/bmt.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>

OCLHelper::OCLHelper(const std::string config_file)
{
    m_ocl_config = std::make_shared<noma::ocl::config>(config_file);
    m_ocl_helper = std::make_shared<noma::ocl::helper>(*m_ocl_config);
    m_ocl_helper->write_device_info(std::cerr);

}

void OCLHelper::CreateProgramFromFile(const std::string kernel_file, const std::string kernel_name, const std::string compile_options)
{
    cl_int err = 0;

    m_Program = m_ocl_helper->create_program_from_file(kernel_file, "", compile_options);
    m_Kernel = cl::Kernel(m_Program, kernel_name.c_str(), &err);
    noma::ocl::error_handler(err, "Error creating kernel: '" + kernel_name + "'.");

}

void OCLHelper::SetArgument(RenderKernelArgument_t argIndex, void* data, size_t size)
{
    cl_int err = m_Kernel.setArg(static_cast<unsigned int>(argIndex), size, data);
    noma::ocl::error_handler(err, "Failed to set kernel argument");
}

cl_ulong OCLHelper::RunKernelTimed(size_t work_items)
{
    noma::ocl::nd_range ndr { { }, // offset
                              { work_items }, // global size
                              { } // local size
    };
    //if (m_ocl_config.opencl_work_group_size() > 0)
    //    ndr.local = { m_ocl_config.opencl_work_group_size() };
    return m_ocl_helper->run_kernel_timed(m_Kernel, ndr);

}

void OCLHelper::ReadBuffer(const cl::Buffer& buffer, void* data, size_t size) const
{
    cl_int err = m_ocl_helper->queue().enqueueReadBuffer(buffer, false, 0, size, data);
    noma::ocl::error_handler(err, "Failed to read buffer");
}
