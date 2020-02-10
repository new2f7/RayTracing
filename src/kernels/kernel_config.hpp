#ifndef RAYTRACING_KERNEL_CONFIG_HPP
#define RAYTRACING_KERNEL_CONFIG_HPP

#include <string>
#include <vector>

struct kernel_config
{
    const std::string name;
    const std::string compile_options;
};

std::vector<kernel_config> get_configs_to_benchmark()
{
    return {
        //TODO: Rename 'default' to 'GGX'?
        {"default", ""},
        {"blinn", " -DBLINN "},
        {"beckmann", " -DBECKMANN "},
        {"algorithm1", " -DALGORITHM1 "},
        {"algorithm1_blinn", " -DALGORITHM1 -DBLINN "},
        {"algorithm1_beckmann", " -DALGORITHM1 -DBECKMANN "}
    };
}

#endif //RAYTRACING_KERNEL_CONFIG_HPP
