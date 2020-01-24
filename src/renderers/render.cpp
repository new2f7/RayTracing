#include "render.hpp"
#include "mathlib/mathlib.hpp"
#include "io/hdr_loader.hpp"
#include "io/store_bmp.hpp"
#include "utils/cl_exception.hpp"
#include <iostream>

static Render g_Render;
Render* render = &g_Render;

void Render::Init()
{
    m_Viewport = std::make_shared<Viewport>(0, 0, 1280, 720);
    m_Camera = std::make_shared<Camera>(m_Viewport);
    m_Scene = std::make_shared<BVHScene>("meshes/dragon.obj", 4);

    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.empty())
    {
        throw std::runtime_error("No OpenCL platforms found");
    }
    
    m_CLContext = std::make_shared<CLContext>(all_platforms[0]);

    std::vector<cl::Device> platform_devices;
    all_platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);
    m_RenderKernel = std::make_shared<CLKernel>("src/kernels/kernel_bvh.cl", platform_devices);

    SetupBuffers();

}

Image image;
void Render::SetupBuffers()
{
    GetCLKernel()->SetArgument(RenderKernelArgument_t::WIDTH, &m_Viewport->width, sizeof(unsigned int));
    GetCLKernel()->SetArgument(RenderKernelArgument_t::HEIGHT, &m_Viewport->height, sizeof(unsigned int));

    cl_int errCode;
    m_OutputBuffer = cl::Buffer(GetCLContext()->GetContext(), CL_MEM_READ_WRITE, GetGlobalWorkSize() * sizeof(float3), 0, &errCode);
    if (errCode)
    {
        throw CLException("Failed to create output buffer", errCode);
    }
    GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_OUT, &m_OutputBuffer, sizeof(cl::Buffer));
    
    m_Scene->SetupBuffers();

    // Texture Buffers
    cl::ImageFormat imageFormat;
    imageFormat.image_channel_order = CL_RGBA;
    imageFormat.image_channel_data_type = CL_FLOAT;

    HDRLoader::Load("textures/Topanga_Forest_B_3k.hdr", image);
    //HDRLoader::Load("textures/studio.hdr", image);

    m_Texture0 = cl::Image2D(GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageFormat, image.width, image.height, 0, image.colors, &errCode);
    if (errCode)
    {
        throw CLException("Failed to create image", errCode);
    }
    GetCLKernel()->SetArgument(RenderKernelArgument_t::TEXTURE0, &m_Texture0, sizeof(cl::Image2D));

}

double Render::GetCurtime() const
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

unsigned int Render::GetGlobalWorkSize() const
{
    if (m_Viewport)
    {
        return m_Viewport->width * m_Viewport->height;
    }
    else
    {
        return 0;
    }
}

std::shared_ptr<CLContext> Render::GetCLContext() const
{
    return m_CLContext;
}

std::shared_ptr<CLKernel> Render::GetCLKernel() const
{
    return m_RenderKernel;
}

void Render::RenderFrame()
{
    m_Camera->Update();

    unsigned int globalWorksize = GetGlobalWorkSize();
    GetCLContext()->ExecuteKernel(GetCLKernel(), globalWorksize);
    GetCLContext()->ReadBuffer(m_OutputBuffer, m_Viewport->pixels, sizeof(float3) * globalWorksize);
    GetCLContext()->Finish();

    StoreBMP::Store("out.bmp", m_Viewport->pixels, m_Viewport->width, m_Viewport->height);
}

void Render::Shutdown()
{

}
