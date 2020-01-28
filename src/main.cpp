#include "renderers/render.hpp"
#include "utils/cl_exception.hpp"
#include "io/benchmark_config.hpp"

int main(int argc, char* argv[])
{
    std::string config_file = "default.cfg";
    std::string result_file = "results.dat";

    if (argc >= 2)
        config_file = argv[1];
    if (argc >= 3)
        result_file = argv[2];

    benchmark_config bm_config(config_file);

    try
    {
        render->Init(config_file, bm_config.benchmark_width(), bm_config.benchmark_height());
    }
    catch (std::exception& ex)
    {
        std::cerr << "Caught exception: " << ex.what() << std::endl;
        return 0;
    }

    for (size_t i = 0; i < bm_config.benchmark_kernel_runs(); ++i)
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
