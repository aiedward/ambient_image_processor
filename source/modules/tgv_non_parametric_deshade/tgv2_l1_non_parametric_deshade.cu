

#include "tgv2_l1.cu"
#include "tgv2_l1_non_parametric_deshade_metrics.cu"

// typedefs....
typedef const unsigned int DimensionSize;

#include <functional>

template<typename Pixel>
using CosineTransformCallback = std::function<void(Pixel* image,
DimensionSize width, DimensionSize height, DimensionSize depth,
Pixel* result, bool is_inverse)>;

template<typename Pixel>
using IterateCallback = std::function<Pixel(const Pixel alpha0, const Pixel alpha1)>;

// kernel forward declarations...

template<typename Pixel>
__global__ void multiply_kernel(
        Pixel* image1, Pixel* image2,
        const uint width, const uint height, const uint depth);

template<typename Pixel>
__global__  void solve_poisson_in_cosine_domain_kernel(Pixel* image,
                              uint width, uint height, uint depth,
                              Pixel* result);

template<typename Pixel>
Pixel* launch_divergence(
        Pixel* dx, Pixel* dy, Pixel* dz,
        Pixel* dxdx, Pixel* dydy, Pixel* dzdz,

        const uint width, const uint height, const uint depth,

        dim3 block_dimension,
        dim3 grid_dimension,
        dim3 grid_dimension_x,
        dim3 grid_dimension_y,
        dim3 grid_dimension_z);

// kernel
template<typename Pixel>
__global__ void subtract_kernel3(
        Pixel* image1, Pixel* image2, Pixel* result,
        const uint voxel_count)
{
    const int index = blockDim.x * blockIdx.x + threadIdx.x;

    if(index >= voxel_count)
        return;

    result[index] = image1[index] - image2[index];
}
template<typename Pixel>
__global__ void multiply_constant_kernel1(
        Pixel* image, Pixel constant,
        const uint voxel_count)
{
    const int index = blockDim.x * blockIdx.x + threadIdx.x;

    if(index >= voxel_count)
        return;

    image[index] = image[index] * constant;
}

// optimization
template<typename Pixel>
void tgv2_l1_non_parametric_deshade_optimize(IterateCallback<Pixel> iterate_callback,
                                             const Pixel alpha_ratio_step_min,
                                             const uint final_iteration_count)
{
    Pixel alpha0 = 2;
    Pixel alpha1 = 1;
    Pixel metric1 = iterate_callback(alpha0, alpha1);
}

template<typename Pixel>
void tgv2_l1_non_parametric_deshade_launch(
        Pixel* f_host,
        DimensionSize width, DimensionSize height, DimensionSize depth,

        const Pixel lambda,
        Pixel* mask_host,

        const uint check_iteration_count,
        const Pixel alpha_ratio_step_min,
        const uint final_iteration_count,

        CosineTransformCallback<Pixel> cosine_transform_callback,
        Pixel** denoised_host,
        Pixel** shading_host,
        Pixel** deshaded_host)
{

  //  CosineTransformCallback<Pixel> cosine_transform_callback = nullptr;

    uint voxel_count;
    dim3 block_dimension;
    dim3 grid_dimension;
    dim3 grid_dimension_x;
    dim3 grid_dimension_y;
    dim3 grid_dimension_z;

    tgv_launch_part1<Pixel>(
        width, height, depth,
        voxel_count,
        block_dimension,
        grid_dimension,
        grid_dimension_x,
        grid_dimension_y,
        grid_dimension_z);

    Pixel* f, *u;
    Pixel* u_previous, *u_bar, *p_x, *p_y, *p_z, *p_xx, *p_yy, *p_zz;

    tgv_launch_part2<Pixel>(f_host,
                            voxel_count, depth,
                            &f, &u,
                            &u_previous, &u_bar, &p_x, &p_y, &p_z, &p_xx, &p_yy, &p_zz);

    Pixel *v_x, *v_y, *v_z;
    Pixel *v_bar_x, *v_bar_y, *v_bar_z;
    Pixel *v_previous_x, *v_previous_y, *v_previous_z;
    Pixel *q_x, *q_y, *q_z, *q_xy, *q_xz, *q_yz;
    Pixel *q_x2, *q_y2, *q_z2, *q_xy2, *q_xz2, *q_yz2;
    Pixel * q_temp;

    tgv_launch_part22<Pixel>(
        voxel_count, depth,
        &v_previous_x, &v_previous_y, &v_previous_z,
        &v_bar_x, &v_bar_y, &v_bar_z,
        &v_x, &v_y, &v_z,
        &q_x, &q_y, &q_z,
        &q_xy, &q_xz, &q_yz,
        &q_x2, &q_y2, &q_z2,
        &q_xy2, &q_xz2, &q_yz2, &q_temp);

    // additional images...
    size_t size = sizeof(Pixel) * voxel_count;
    Pixel * mask = nullptr;
    if(mask_host != nullptr)
    {
       cudaCheckError( cudaMallocManaged(&mask, size) )
       cudaCheckError( cudaMemcpy(mask, mask_host, size, cudaMemcpyHostToDevice) )
    }
    Pixel* shading_image, *deshaded_image;
    cudaCheckError( cudaMallocManaged(&shading_image, size) )
    cudaCheckError( cudaMallocManaged(&deshaded_image, size) )
    cudaCheckError( cudaDeviceSynchronize() );

    // algorithm variables..
    const Pixel tau = 1.0 / std::sqrt(20.0);
    const Pixel sigma = tau;
    const Pixel theta = 1;

    IterateCallback<Pixel> iterate_callback = [=](const Pixel alpha0, const Pixel alpha1) {
        // algorithm begin
        tgv2_l1_non_parametric_deshade_zero_init(
                p_x, p_y, p_z,
                p_xx, p_yy, p_zz,
                v_x, v_y, v_z,
                v_bar_x, v_bar_y, v_bar_z,
                q_x, q_y, q_z,
                q_xy, q_xz, q_yz,
                f, u, u_bar, voxel_count, depth,
                block_dimension,
                grid_dimension);

        return tgv2_l1_non_parametric_deshade_iterate(
                width, height, depth,
                block_dimension,
                grid_dimension,
                grid_dimension_x,
                grid_dimension_y,
                grid_dimension_z,

                cosine_transform_callback,

                check_iteration_count,

                lambda,
                alpha0,
                alpha1,

                theta,
                tau,
                sigma,

                f,
                u,
                u_previous,
                u_bar,

                mask,

                p_x,
                p_y,
                p_z,
                p_xx,
                p_yy,
                p_zz,

                v_x,
                v_y,
                v_z,
                v_previous_x,
                v_previous_y,
                v_previous_z,
                v_bar_x,
                v_bar_y,
                v_bar_z,

                q_x,
                q_y,
                q_z,
                q_xy,
                q_xz,
                q_yz,
                q_x2,
                q_y2,
                q_z2,
                q_xy2,
                q_xz2,
                q_yz2,
                q_temp,

                shading_image,
                deshaded_image);
    };

    // optimization ...
    tgv2_l1_non_parametric_deshade_optimize(iterate_callback, alpha_ratio_step_min, final_iteration_count);


    // free
    if(mask != nullptr)
    cudaFree(mask);

    *denoised_host = new Pixel[voxel_count];
    tgv_launch_part3<Pixel>(
            *denoised_host,
            voxel_count, depth,
            u_previous, u_bar,
            p_x, p_y, p_z,
            p_xx, p_yy, p_zz,
            f, u);
    tgv_launch_part32<Pixel>( depth,
                              v_bar_x, v_bar_y, v_bar_z,
                              v_previous_x, v_previous_y, v_previous_z,
                              v_x, v_y, v_z,
                              q_x, q_y, q_z,
                              q_xy, q_xz, q_yz,
                              q_x2, q_y2, q_z2,
                              q_xy2, q_xz2, q_yz2, q_temp);

    if(mask != nullptr)
    {
        multiply_kernel<<<grid_dimension, block_dimension>>>(
             deshaded_image, mask, width, height, depth);
    }

    // copy
    *deshaded_host = new Pixel[voxel_count];
    *shading_host = new Pixel[voxel_count];
    cudaCheckError( cudaMemcpy(*deshaded_host, deshaded_image, size, cudaMemcpyDeviceToHost) );
    cudaCheckError( cudaMemcpy(*shading_host, shading_image, size, cudaMemcpyDeviceToHost) );
    cudaCheckError( cudaDeviceSynchronize() );
    cudaFree(deshaded_image);
    cudaFree(shading_image);
}


template<typename Pixel>
void tgv2_l1_non_parametric_deshade_zero_init(
        Pixel* p_x, Pixel* p_y, Pixel* p_z,
        Pixel* p_xx, Pixel* p_yy, Pixel* p_zz,
        Pixel* v_x, Pixel* v_y, Pixel* v_z,
        Pixel* v_bar_x, Pixel* v_bar_y, Pixel* v_bar_z,
        Pixel* q_x, Pixel* q_y, Pixel* q_z,
        Pixel* q_xy, Pixel* q_xz, Pixel* q_yz,
        Pixel* f, Pixel* u, Pixel* u_bar,
        DimensionSize voxel_count,
        DimensionSize depth,
        dim3 block_dimension,
        dim3 grid_dimension)
{
    zeroInit<<<grid_dimension, block_dimension>>>(p_x, p_y, p_z,
                                                  p_xx, p_yy, p_zz,
                                                  voxel_count, depth);
    zeroInit<<<grid_dimension, block_dimension>>>(v_x, v_y, v_z,
                                                  v_bar_x, v_bar_y, v_bar_z,
                                                  voxel_count, depth);
    zeroInit2<<<grid_dimension, block_dimension>>>(q_x, q_y, q_z,
                                                   q_xy, q_xz, q_yz,
                                                   voxel_count, depth);
    clone2<<<grid_dimension, block_dimension>>>(f, u, u_bar, voxel_count);
    cudaCheckError( cudaDeviceSynchronize() );
}

template<typename Pixel>
void tgv2_l1_non_parametric_deshade_calculate_shading_and_deshaded(
        uint width, uint height, uint depth,
        dim3 block_dimension,
        dim3 grid_dimension,
        dim3 grid_dimension_x,
        dim3 grid_dimension_y,
        dim3 grid_dimension_z,

        Pixel* u,

        Pixel* v_x,
        Pixel* v_y,
        Pixel* v_z,
        Pixel* temp_x,
        Pixel* temp_y,
        Pixel* temp_z,

        CosineTransformCallback<Pixel> cosine_transform_callback,

        Pixel* shading_image,
        Pixel* deshaded_image)
{
    // uses deshaded_image as a temp variable

    launch_divergence(v_x, v_y, v_z,
                      deshaded_image, temp_y, temp_z,
                      width, height, depth,
                      block_dimension,
                      grid_dimension,
                      grid_dimension_x,
                      grid_dimension_y,
                      grid_dimension_z);

    cosine_transform_callback(
                deshaded_image,
                width, height, depth,
                shading_image, false);

    solve_poisson_in_cosine_domain_kernel<<<grid_dimension, block_dimension>>>(
      shading_image, width, height, depth,
      deshaded_image);
    cudaCheckError( cudaDeviceSynchronize() );

    cosine_transform_callback(
                deshaded_image,
                width, height, depth,
                shading_image, true);  // inverse

    const uint voxel_count = width * height * depth;
    Pixel constant = 0.25 * 0.5 / voxel_count; // multiply by the idct normalization factor
    multiply_constant_kernel1<<<grid_dimension, block_dimension>>>(
      shading_image, constant, voxel_count);
    cudaCheckError( cudaDeviceSynchronize() );

    subtract_kernel3<<<grid_dimension, block_dimension>>>(
      u, shading_image, deshaded_image, voxel_count);
    cudaCheckError( cudaDeviceSynchronize() );
}

template<typename Pixel>
Pixel tgv2_l1_non_parametric_deshade_iterate(
        DimensionSize width, DimensionSize height, DimensionSize depth,
        dim3 block_dimension,
        dim3 grid_dimension,
        dim3 grid_dimension_x,
        dim3 grid_dimension_y,
        dim3 grid_dimension_z,

        CosineTransformCallback<Pixel> cosine_transform_callback,

        const uint check_iteration_count,

        Pixel lambda,
        const Pixel alpha0,
        const Pixel alpha1,

        const Pixel theta,
        const Pixel tau,
        const Pixel sigma,

        Pixel* f,
        Pixel* u,
        Pixel* u_previous,
        Pixel* u_bar,

        Pixel* mask,

        Pixel* p_x,
        Pixel* p_y,
        Pixel* p_z,
        Pixel* p_xx,
        Pixel* p_yy,
        Pixel* p_zz,

        Pixel* v_x,
        Pixel* v_y,
        Pixel* v_z,
        Pixel* v_previous_x,
        Pixel* v_previous_y,
        Pixel* v_previous_z,
        Pixel* v_bar_x,
        Pixel* v_bar_y,
        Pixel* v_bar_z,

        Pixel* q_x,
        Pixel* q_y,
        Pixel* q_z,
        Pixel* q_xy,
        Pixel* q_xz,
        Pixel* q_yz,
        Pixel* q_x2,
        Pixel* q_y2,
        Pixel* q_z2,
        Pixel* q_xy2,
        Pixel* q_xz2,
        Pixel* q_yz2,
        Pixel* q_temp,

        Pixel* shading_image,
        Pixel* deshaded_image)
{
    uint voxel_count = width * height * depth;

    /*
    Pixel* shading_image_previous = nullptr;
    Pixel metric_previous;
    Pixel metric_sum = 0;
    */

    for(uint iteration_index = 0; iteration_index < check_iteration_count; iteration_index++)
    {
        tgv_launch_forward_differences<Pixel>(u_bar,
                                              p_x,p_y,p_z,
                                              width, height, depth,
                                              block_dimension,
                                              grid_dimension_x,
                                              grid_dimension_y,
                                              grid_dimension_z);

        tgv_kernel_part22<<<grid_dimension, block_dimension>>>( v_bar_x, v_bar_y, v_bar_z,
                                                                p_x, p_y, p_z,
                                                                p_xx, p_yy, p_zz,
                                                                sigma, alpha1, u_previous, u,
                                                                width, height, depth);
        cudaCheckError( cudaDeviceSynchronize() );

        tgv_launch_backward_differences<Pixel>(
                    p_x, p_y, p_z,
                    p_xx, p_yy, p_zz,
                    width, height, depth,
                    block_dimension,
                    grid_dimension_x,
                    grid_dimension_y,
                    grid_dimension_z);
        cudaCheckError( cudaDeviceSynchronize() );

        tgv_kernel_part4_tgv2_l1<<<grid_dimension, block_dimension>>>(
                                                                        p_x, p_y, p_z,
                                                                        tau, u, f,
                                                                        lambda,
                                                                        u_previous, theta, u_bar,
                                                                        width, height, depth);
        cudaCheckError( cudaDeviceSynchronize() );

        /*
        % dual update q
        q = q + sigma*nabla_second*v_bar;
        norm_q = sqrt(q(1:N).^2 + q(N+1:2*N).^2 + q(2*N+1:3*N).^2 + ... % main diagonal
            2*q(3*N+1:4*N).^2 + 2*q(4*N+1:5*N).^2 + 2*q(5*N+1:6*N).^2); % off diagonal
        q = q./max(1, repmat(norm_q, 6, 1)/alpha0);

            v_old = v;
        */
        tgv_launch_gradient2<Pixel>(
                    v_bar_x, v_bar_y, v_bar_z,
                    q_x2,q_y2,q_z2,
                    q_xy2,q_xz2,q_yz2, q_temp,
                    width, height, depth,
                    block_dimension,
                    grid_dimension,
                    grid_dimension_x,
                    grid_dimension_y,
                    grid_dimension_z);

        tgv_kernel_part5<<<grid_dimension, block_dimension>>>(
                                                                v_x, v_y, v_z,
                                                                v_previous_x, v_previous_y, v_previous_z,
                                                                q_x2,q_y2,q_z2,
                                                                q_xy2,q_xz2,q_yz2,
                                                                q_x, q_y, q_z,
                                                                q_xy, q_xz, q_yz,
                                                                sigma, alpha0,
                                                                width, height, depth);
        cudaCheckError( cudaDeviceSynchronize() );

        tgv_launch_divergence2<Pixel>(
                    q_x, q_y, q_z,
                    q_xy, q_xz, q_yz,
                    q_x2, q_y2, q_z2,
                    q_temp,
                    width, height, depth,
                    block_dimension,
                    grid_dimension,
                    grid_dimension_x,
                    grid_dimension_y,
                    grid_dimension_z);
        cudaCheckError( cudaDeviceSynchronize() );


        /*
            % primal update v
            v = v - tau * (nabla_second_t * q - p);
            v_bar = v + theta*(v - v_old);
            */

        tgv_kernel_part6<<<grid_dimension, block_dimension>>>(
                                                                v_x, v_y, v_z,
                                                                q_x2, q_y2, q_z2,
                                                                p_xx, p_yy, p_zz,
                                                                v_previous_x, v_previous_y, v_previous_z,
                                                                v_bar_x, v_bar_y, v_bar_z,
                                                                tau, theta,
                                                                width, height, depth);
        cudaCheckError( cudaDeviceSynchronize() );

        tgv2_l1_non_parametric_deshade_calculate_shading_and_deshaded(
                    width, height, depth,
                    block_dimension,
                    grid_dimension,
                    grid_dimension_x, grid_dimension_y, grid_dimension_z,
                    u,
                    v_x, v_y, v_z,
                    p_x, p_y, p_z,

                    cosine_transform_callback,
                    shading_image,
                    deshaded_image);

        /*
        if(shading_image_before != nullptr)
        {
            // convergence check
            float metric = normalized_cross_correlation(shading_image, shading_image_before, voxel_count);
         //   float sad = sum_of_absolute_differences(shading_image, shading_image_before, voxel_count);
            metric_sum += abs(metric - metric_before);

            metric_before = metric;
        }
        */
    }

    return 0;
    //return metric_sum;
}


// generate the algorithm explicitly for...

template void tgv2_l1_non_parametric_deshade_launch(float* f_host,
DimensionSize width, DimensionSize height, DimensionSize depth,

const float lambda,
float* mask_host,

const uint check_iteration_count,
const float alpha_ratio_step_min,
const uint final_iteration_count,

CosineTransformCallback<float> cosine_transform_callback,
float** denoised_host,
float** shading_host,
float** deshaded_host);

template void tgv2_l1_non_parametric_deshade_launch(double* f_host,
DimensionSize width, DimensionSize height, DimensionSize depth,

const double lambda,
double* mask_host,

const uint check_iteration_count,
const double alpha_ratio_step_min,
const uint final_iteration_count,

CosineTransformCallback<double> cosine_transform_callback,
double** denoised_host,
double** shading_host,
double** deshaded_host);
