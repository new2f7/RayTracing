#include "renderers/render.hpp"
#include "utils/cl_exception.hpp"

int main()
{
    try
    {
        render->Init();
    }
    catch (std::exception& ex)
    {
        std::cerr << "Caught exception: " << ex.what() << std::endl;
        return 0;
    }

    size_t kernel_runs = 10;
    for (size_t i = 0; i < kernel_runs; ++i)
    {
        try
        {
            render->RenderFrame();
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Caught exception: " << ex.what() << std::endl;
            return 0;
        }
    }

    render->Shutdown();

    return EXIT_SUCCESS;

}
