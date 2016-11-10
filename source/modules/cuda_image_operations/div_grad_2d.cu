
#include "unary_operation.cu"
#include "tgv_common_2d.cu"

template<typename Pixel>
void launch_divergence_2d(
        Pixel* dx, Pixel* dy,
        Pixel* dxdx, Pixel* dydy,

        const uint width, const uint height,

        dim3 block_dimension,
        dim3 grid_dimension,
        dim3 grid_dimension_x,
        dim3 grid_dimension_y);

template<typename Pixel>
void tgv_launch_forward_differences_2d(Pixel* u_bar,
        Pixel* p_x, Pixel* p_y,
        uint width, uint height,
        dim3 block_dimension,
        dim3 grid_dimension_x,
        dim3 grid_dimension_y);

template<typename Pixel>
Pixel* div_grad_2d_kernel_launch(Pixel* image_host,
                  uint width, uint height)
{
    dim3 block_dimension;
    dim3 grid_dimension;
    Pixel* image;

    const uint depth = 1;
    unary_operation_part1(image_host,
                      width, height, depth,
                      &image,
                      block_dimension, grid_dimension);

    Pixel* grad_x, *grad_y;
    Pixel *dgrad_y;
    uint voxel_count = width*height*depth;
    size_t size = sizeof(Pixel) * voxel_count;
    cudaCheckError( cudaMalloc(&grad_x, size) )
    cudaCheckError( cudaMalloc(&grad_y, size) )

    cudaCheckError( cudaMalloc(&dgrad_y, size) )

    dim3 grid_dimension_x = dim3((depth*height + block_dimension.x - 1) / block_dimension.x);
    dim3 grid_dimension_y = dim3((depth*width + block_dimension.x - 1) / block_dimension.x);

    tgv_launch_forward_differences_2d(image, grad_x, grad_y, width, height,
                                      block_dimension, grid_dimension_x, grid_dimension_y);

    launch_divergence_2d(grad_x, grad_y,
                      image, dgrad_y,
                      width, height,
                      block_dimension,
                      grid_dimension,
                      grid_dimension_x,
                      grid_dimension_y);

    cudaFree(grad_x);
    cudaFree(grad_y);
    cudaFree(dgrad_y);

    return unary_operation_part2(image, width, height, depth);
}

template float* div_grad_2d_kernel_launch(float* image,
                  uint width, uint height);
template double* div_grad_2d_kernel_launch(double* image,
                  uint width, uint heighth);

