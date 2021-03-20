#include "render.hpp"
#include "mathlib/mathlib.hpp"
#include "io/hdr_loader.hpp"
#include "io/store_bmp.hpp"
#include "utils/cl_exception.hpp"
#include <iostream>

static Render g_Render;
Render* render = &g_Render;

void Render::Init(std::string config_file, size_t width, size_t height)
{
    m_OCLHelper = std::make_shared<OCLHelper>(config_file);
    m_OCLHelper->CreateProgramFromFile("src/kernels/kernel_bvh.cl", "KernelEntry");

    m_Viewport = std::make_shared<Viewport>(width, height);
    m_Camera = std::make_shared<Camera>();
    m_Scene = std::make_shared<BVHScene>("meshes/dragon.obj", 4);

    SetupBuffers();
}

Image image;
void Render::SetupBuffers()
{
    m_OCLHelper->SetArgument(RenderKernelArgument_t::WIDTH, &m_Viewport->width, sizeof(unsigned int));
    m_OCLHelper->SetArgument(RenderKernelArgument_t::HEIGHT, &m_Viewport->height, sizeof(unsigned int));

    m_OutputBuffer = m_OCLHelper->GetOCLHelper()->create_buffer(CL_MEM_READ_WRITE, GetGlobalWorkSize() * sizeof(float) * 4);
    std::cout << "OutputBuffer size: " << float(GetGlobalWorkSize() * sizeof(float) * 4) / (1024.0f * 1024.0f) << " MiB" << std::endl;
    m_OCLHelper->SetArgument(RenderKernelArgument_t::BUFFER_OUT, &m_OutputBuffer, sizeof(cl::Buffer));
    
    m_Scene->SetupBuffers();

    // Texture Buffers
    cl::ImageFormat imageFormat;
    imageFormat.image_channel_order = CL_RGBA;
    imageFormat.image_channel_data_type = CL_FLOAT;

    HDRLoader::Load("textures/Topanga_Forest_B_3k.hdr", image);
    //HDRLoader::Load("textures/studio.hdr", image);

    cl_int errCode;
    m_Texture0 = cl::Image2D(m_OCLHelper->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageFormat, image.width, image.height, 0, image.colors, &errCode);
    std::cout << "Texture0 size: " << float(image.width * image.height * sizeof(float) * 4) / (1024.0f * 1024.0f) << " MiB" << std::endl;
    noma::ocl::error_handler(errCode, "Failed to create image");

    m_OCLHelper->SetArgument(RenderKernelArgument_t::TEXTURE0, &m_Texture0, sizeof(cl::Image2D));

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

cl_ulong Render::RenderFrame()
{

    m_Camera->Update();

    cl_ulong t = m_OCLHelper->RunKernelTimed(GetGlobalWorkSize());

#ifdef STORE_BMP
    m_OCLHelper->ReadBuffer(m_OutputBuffer, m_Viewport->pixels, sizeof(float) * 4 * GetGlobalWorkSize());
    std::string filename = "out_" + std::to_string(m_Camera->GetFrameCount()) + ".bmp";
    StoreBMP::Store(filename.c_str(), m_Viewport);
#endif

    return t;
}

void Render::Shutdown()
{

}

std::shared_ptr<OCLHelper> Render::GetOCLHelper() const
{
    return m_OCLHelper;
}
