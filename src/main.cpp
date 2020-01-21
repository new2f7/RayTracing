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

    try
    {
        render->RenderFrame();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Caught exception: " << ex.what() << std::endl;
        return 0;
    }

    render->Shutdown();

    return EXIT_SUCCESS;

}
