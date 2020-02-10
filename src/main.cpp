#include <fstream>

#include "noma/bmt/bmt.hpp"

#include "renderers/render.hpp"
#include "utils/cl_exception.hpp"
#include "io/benchmark_config.hpp"
#include "kernels/kernel_config.hpp"

int main(int argc, char* argv[])
{
    std::string config_file = "default.cfg";
    std::string result_file = "results.dat";

    if (argc >= 2)
        config_file = argv[1];
    if (argc >= 3)
        result_file = argv[2];

    benchmark_config bm_config(config_file);

    // prepare output file
    std::ofstream of(result_file);

    // header
    of << "name" << "\t"
       << noma::bmt::statistics::header_string(false) << "\t"
       //<< "kernel_warmups" << "\t"
       << "kernel_runs" << "\t"
       << "width" << "\t"
       << "height" << std::endl;

    // suffix with all same values for every benchmark
    std::stringstream constant_values;
    constant_values //<< kernel_warmups << "\t"
                    << bm_config.benchmark_kernel_runs() << "\t"
                    << bm_config.benchmark_width() << "\t"
                    << bm_config.benchmark_height();

    const std::vector<kernel_config> kernels_configs = get_configs_to_benchmark();

    for(auto& kc : kernels_configs)
    {
        try
        {
            render->Init(config_file, bm_config.benchmark_width(), bm_config.benchmark_height(), kc.compile_options);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Caught exception: " << ex.what() << std::endl;
            return 0;
        }

        noma::bmt::statistics kernel_stats(bm_config.benchmark_kernel_runs(), 0);

        for (size_t i = 0; i < bm_config.benchmark_kernel_runs(); ++i)
        {
            try
            {
                cl_ulong t = render->RenderFrame();
                kernel_stats.add(noma::bmt::duration(static_cast<noma::bmt::rep>(t)));
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Caught exception: " << ex.what() << std::endl;
                return 0;
            }

        }

        // print summary to std::cout
        std::cout << "Time for '" << kc.name << "' kernel: "
                  << std::chrono::duration_cast<noma::bmt::seconds>(kernel_stats.sum()).count() << " s"
                  << ", average frame time: "
                  << std::chrono::duration_cast<noma::bmt::milliseconds>(kernel_stats.average()).count() << " ms"
                  << ", frames per second: "
                  << bm_config.benchmark_kernel_runs() / std::chrono::duration_cast<noma::bmt::seconds>(kernel_stats.sum()).count()
                  << std::endl;

        // write details into file
        of << kc.name << '\t'
           << kernel_stats.string() << '\t'
           << constant_values.str() << std::endl;

        render->Shutdown();
    }

    return EXIT_SUCCESS;

}
