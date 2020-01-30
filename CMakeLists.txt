cmake_minimum_required(VERSION 3.8)

project(RayTracing CXX)

set(CONTEXT_SOURCES
    src/ocl_helper/ocl_helper.cpp
    src/ocl_helper/ocl_helper.hpp
)

set(IO_SOURCES
    src/io/hdr_loader.cpp
    src/io/hdr_loader.hpp
    src/io/store_bmp.cpp
    src/io/store_bmp.hpp
    src/io/benchmark_config.cpp
    src/io/benchmark_config.hpp
)

set(KERNELS_SOURCES
    src/kernels/kernel_bvh.cl
)

set(MATHLIB_SOURCES
    src/mathlib/mathlib.cpp
    src/mathlib/mathlib.hpp
)

set(RENDERERS_SOURCES
    src/renderers/render.cpp
    src/renderers/render.hpp
)

set(SCENE_SOURCES
    src/scene/camera.cpp
    src/scene/camera.hpp
    src/scene/scene.cpp
    src/scene/scene.hpp
)

set(UTILS_SOURCES
    src/utils/cl_exception.hpp
    src/utils/shared_structs.hpp
    src/utils/viewport.hpp
)

set(MAIN_SOURCES
    src/main.cpp
)

set(SOURCES
    ${CONTEXT_SOURCES}
    ${IO_SOURCES}
    ${KERNELS_SOURCES}
    ${MATHLIB_SOURCES}
    ${RENDERERS_SOURCES}
    ${SCENE_SOURCES}
    ${UTILS_SOURCES}
    ${MAIN_SOURCES}
)

add_subdirectory(thirdparty/misc/ ${CMAKE_CURRENT_BINARY_DIR}/build.noma_misc)
add_subdirectory(thirdparty/bmt/  ${CMAKE_CURRENT_BINARY_DIR}/build.noma_bmt)
add_subdirectory(thirdparty/typa/ ${CMAKE_CURRENT_BINARY_DIR}/build.noma_typa)
add_subdirectory(thirdparty/ocl/  ${CMAKE_CURRENT_BINARY_DIR}/build.noma_ocl)

add_executable(RayTracing ${SOURCES})
find_package(OpenCL REQUIRED)
target_include_directories(RayTracing PUBLIC "${RayTracing_SOURCE_DIR}/src")
target_link_libraries(RayTracing PUBLIC OpenCL::OpenCL noma_ocl)
set_target_properties(RayTracing PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${RayTracing_SOURCE_DIR}
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
