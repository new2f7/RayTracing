#include "render.hpp"
#include "mathlib/mathlib.hpp"
#include "io/image_loader.hpp"
#include "utils/cl_exception.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

static Render g_Render;
Render* render = &g_Render;

// From https://devblogs.nvidia.com/egl-eye-opengl-visualization-without-x-server/
static const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_NONE
};

static const int pbufferWidth = 1080;
static const int pbufferHeight = 1920;

static const EGLint pbufferAttribs[] = {
    EGL_WIDTH, pbufferWidth,
    EGL_HEIGHT, pbufferHeight,
    EGL_NONE,
};

void Render::InitEGL()
{
    // From https://devblogs.nvidia.com/egl-eye-opengl-visualization-without-x-server/
    // 1. Initialize EGL
    m_EGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint major, minor;

    eglInitialize(m_EGLDisplay, &major, &minor);

    // 2. Select an appropriate configuration
    EGLint numConfigs;
    EGLConfig eglCfg;

    eglChooseConfig(m_EGLDisplay, configAttribs, &eglCfg, 1, &numConfigs);

    // 3. Create a surface
    EGLSurface eglSurf = eglCreatePbufferSurface(m_EGLDisplay, eglCfg, pbufferAttribs);

    // 4. Bind the API
    eglBindAPI(EGL_OPENGL_API);

    // 5. Create a context and make it current
    m_EGLContext = eglCreateContext(m_EGLDisplay, eglCfg, EGL_NO_CONTEXT, NULL);

    eglMakeCurrent(m_EGLDisplay, eglSurf, eglSurf, m_EGLContext);
}

void Render::Init()
{
    InitEGL();

    m_Viewport = std::make_shared<Viewport>(0, 0, 1280, 720);
    m_Camera = std::make_shared<Camera>(m_Viewport);
#ifdef BVH_INTERSECTION
    m_Scene = std::make_shared<BVHScene>("meshes/dragon.obj", 4);
#else
    m_Scene = std::make_shared<UniformGridScene>("meshes/room.obj");
#endif
    
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.empty())
    {
        throw std::runtime_error("No OpenCL platforms found");
    }
    
    m_CLContext = std::make_shared<CLContext>(all_platforms[0]);

    std::vector<cl::Device> platform_devices;
    all_platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &platform_devices);
#ifdef BVH_INTERSECTION
    m_RenderKernel = std::make_shared<CLKernel>("src/kernels/kernel_bvh.cl", platform_devices);
#else
    m_RenderKernel = std::make_shared<CLKernel>("src/kernels/kernel_grid.cl", platform_devices);
#endif

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

double Render::GetDeltaTime() const
{
    return GetCurtime() - m_PreviousFrameTime;
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

EGLDisplay Render::GetEGLDisplay() const
{
    return m_EGLDisplay;
}

EGLContext Render::GetEGLContext() const
{
    return m_EGLContext;
}

std::shared_ptr<CLContext> Render::GetCLContext() const
{
    return m_CLContext;
}

std::shared_ptr<CLKernel> Render::GetCLKernel() const
{
    return m_RenderKernel;
}

void Render::FrameBegin()
{
    m_StartFrameTime = GetCurtime();
}

void Render::FrameEnd()
{
    m_PreviousFrameTime = m_StartFrameTime;
}

void Render::RenderFrame()
{
    FrameBegin();

    glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //m_Camera->Update();

    //if (m_Camera->GetFrameCount() > 64) return;

    unsigned int globalWorksize = GetGlobalWorkSize();
    GetCLContext()->ExecuteKernel(GetCLKernel(), globalWorksize);
    GetCLContext()->ReadBuffer(m_OutputBuffer, m_Viewport->pixels, sizeof(float3) * globalWorksize);
    GetCLContext()->Finish();

    glDrawPixels(m_Viewport->width, m_Viewport->height, GL_RGBA, GL_FLOAT, m_Viewport->pixels);
        
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.0, 1280.0 / 720.0, 1, 1024);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float3 eye = m_Camera->GetOrigin();
    float3 center = m_Camera->GetFrontVector() + eye;
    gluLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, m_Camera->GetUpVector().x, m_Camera->GetUpVector().y, m_Camera->GetUpVector().z);

    //m_Scene->DrawDebug();

    glFinish();

    SwapBuffers(m_DisplayContext);

    FrameEnd();
}

void Render::Shutdown()
{
    // From https://devblogs.nvidia.com/egl-eye-opengl-visualization-without-x-server/
    // 6. Terminate EGL when finished
    eglTerminate(m_EGLDisplay);
}
