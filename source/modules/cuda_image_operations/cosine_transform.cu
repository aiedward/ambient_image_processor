/*
    Ambient Image Processor - A tool to perform several imaging tasks
    
    Copyright (C) 2016 Josef Koller

    https://github.com/josefkoller/ambient_image_processor    
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "cuda_helper.cuh"

template<typename Pixel>
__global__  void cosine_transform_kernel_3D(Pixel* image,
                              uint width, uint height, uint depth,
                              Pixel* result)
{
    const int index = blockDim.x * blockIdx.x + threadIdx.x;

    if(index >= width * height * depth)
        return;

    const int z = floorf(index / (width*height));
    int index_rest = index - z * (width*height);
    const int y = floorf(index_rest / width);
    index_rest = index_rest - y * width;
    const int x = index_rest;

    const Pixel widthf = width;
    const Pixel heightf = height;
    const Pixel depthf = depth;

    result[index] = 0;
    for(uint z2 = 0; z2 < depth; z2++)
    {
        for(uint y2 = 0; y2 < height; y2++)
        {
            for(uint x2 = 0; x2 < width; x2++)
            {
                uint index2 = z2 * width*height + x2 + y2 * width;
                result[index] += image[index2]
                        * cospi((x2 + 0.5) * x/widthf)
                        * cospi((y2 + 0.5) * y/heightf)
                        * cospi((z2 + 0.5) * z/depthf);
            }
        }
    }
}

template<typename Pixel>
__global__  void cosine_transform_kernel_2D(Pixel* image,
                              uint width, uint height,
                              Pixel* result)
{
    const int index = blockDim.x * blockIdx.x + threadIdx.x;

    if(index >= width * height)
        return;

    const int y = floorf(index / width);
    const int x = index - y * width;

    const Pixel widthf = width;
    const Pixel heightf = height;

    result[index] = 0;
    for(uint y2 = 0; y2 < height; y2++)
    {
        for(uint x2 = 0; x2 < width; x2++)
        {
            uint index2 = x2 + y2 * width;
            result[index] += image[index2]
                    * cospi((x2 + 0.5) * x/widthf)
                    * cospi((y2 + 0.5) * y/heightf);
        }
    }
}

template<typename Pixel>
__global__  void cosine_transform_kernel_x_2D(Pixel* image,
                              uint width, uint height,
                              Pixel* result)
{
    const int index = blockDim.x * blockIdx.x + threadIdx.x;

    if(index >= width * height)
        return;

    const int y = floorf(index / width);
    const int x = index - y * width;

    const Pixel size = width;

    result[index] = 0;
    for(uint x2 = 0; x2 < width; x2++)
    {
        uint index2 = x2 + y * width;
        result[index] += image[index2]
                * cospi((x2 + 0.5) * x/size);
    }
 //   result[index] *= 2;
}

template<typename Pixel>
__global__  void cosine_transform_kernel_y_2D(Pixel* image,
                              uint width, uint height,
                              Pixel* result)
{
    const int index = blockDim.x * blockIdx.x + threadIdx.x;

    if(index >= width * height)
        return;

    const int y = floorf(index / width);
    const int x = index - y * width;

    const Pixel size = height;

    result[index] = 0;
    for(uint y2 = 0; y2 < height; y2++)
    {
        uint index2 = x + y2 * width;
        result[index] += image[index2]
                * cospi((y2 + 0.5) * y/size);
    }
    result[index] *= 8;
}

template<typename Pixel>
__global__  void cosine_transform_kernel_x_3D(Pixel* image,
                              uint width, uint height, uint depth,
                              Pixel* result)
{
    const int index = blockDim.x * blockIdx.x + threadIdx.x;

    if(index >= width * height * depth)
        return;

    const int z = floorf(index / (width*height));
    int index_rest = index - z * (width*height);
    const int y = floorf(index_rest / width);
    index_rest = index_rest - y * width;
    const int x = index_rest;

    const Pixel size = width;

    result[index] = 0;
    for(uint x2 = 0; x2 < width; x2++)
    {
        uint index2 = z * width*height + x2 + y * width;
        result[index] += image[index2]
                * cospi((x2 + 0.5) * x/size);
    }
 //   result[index] *= 2;
}

template<typename Pixel>
__global__  void cosine_transform_kernel_y_3D(Pixel* image,
                              uint width, uint height, uint depth,
                              Pixel* result)
{
    const int index = blockDim.x * blockIdx.x + threadIdx.x;

    if(index >= width * height * depth)
        return;

    const int z = floorf(index / (width*height));
    int index_rest = index - z * (width*height);
    const int y = floorf(index_rest / width);
    index_rest = index_rest - y * width;
    const int x = index_rest;

    const Pixel size = height;

    result[index] = 0;
    for(uint y2 = 0; y2 < height; y2++)
    {
        uint index2 = z * width*height + x + y2 * width;
        result[index] += image[index2]
                * cospi((y2 + 0.5) * y/size);
    }
 //   result[index] *= 2;
}

template<typename Pixel>
__global__  void cosine_transform_kernel_z_3D(Pixel* image,
                              uint width, uint height, uint depth,
                              Pixel* result)
{
    const int index = blockDim.x * blockIdx.x + threadIdx.x;

    if(index >= width * height * depth)
        return;

    const int z = floorf(index / (width*height));
    int index_rest = index - z * (width*height);
    const int y = floorf(index_rest / width);
    index_rest = index_rest - y * width;
    const int x = index_rest;

    const Pixel size = depth;

    result[index] = 0;
    for(uint z2 = 0; z2 < depth; z2++)
    {
        uint index2 = z2 * width*height + x + y * width;
        result[index] += image[index2]
                * cospi((z2 + 0.5) * z/size);
    }
    result[index] *= 8;
}

template<typename Pixel>
Pixel* cosine_transform_kernel_launch(Pixel* image_host,
                              uint width, uint height, uint depth)
{
    int cuda_device_count;
    cudaCheckError( cudaGetDeviceCount(&cuda_device_count) );

//    printf("found %d cuda devices.\n", cuda_device_count);

    uint voxel_count = width*height*depth;
    dim3 block_dimension(CUDA_BLOCK_DIMENSON);
    dim3 grid_dimension((voxel_count + block_dimension.x - 1) / block_dimension.x);
    dim3 grid_dimension_x((width + block_dimension.x - 1) / block_dimension.x);
    dim3 grid_dimension_y((height + block_dimension.x - 1) / block_dimension.x);

    Pixel* image, *result;

    size_t size = sizeof(Pixel) * voxel_count;
    cudaCheckError( cudaMalloc(&image, size) )
    cudaCheckError( cudaMemcpy(image, image_host, size, cudaMemcpyHostToDevice) )

    cudaCheckError( cudaMalloc(&result, size) )
    cudaCheckError( cudaDeviceSynchronize() );

    if(depth == 1)
    {
      //  cosine_transform_kernel_2D<<<grid_dimension, block_dimension>>>(
      //    image, width, height, result);

        // separable product
        cosine_transform_kernel_x_2D<<<grid_dimension, block_dimension>>>(
          image, width, height,
          result);
        cosine_transform_kernel_y_2D<<<grid_dimension, block_dimension>>>(
          result, width, height,
          image);
        // swap
        Pixel* temp = result;
        result = image;
        image = temp;
    }
    else
    {
     //   cosine_transform_kernel_3D<<<grid_dimension, block_dimension>>>(
     //     image, width, height, depth,
      //    result);

        // separable product
        cosine_transform_kernel_x_3D<<<grid_dimension, block_dimension>>>(
          image, width, height, depth,
          result);
        cosine_transform_kernel_y_3D<<<grid_dimension, block_dimension>>>(
          result, width, height, depth,
          image);
        cosine_transform_kernel_z_3D<<<grid_dimension, block_dimension>>>(
          image, width, height, depth,
          result);
    }
    cudaCheckError( cudaDeviceSynchronize() );

    Pixel* result_host = new Pixel[voxel_count];
    cudaCheckError( cudaMemcpy(result_host, result, size, cudaMemcpyDeviceToHost) );
    cudaCheckError( cudaDeviceSynchronize() );

    cudaFree(image);
    cudaFree(result);

    return result_host;
}

template float* cosine_transform_kernel_launch(float* image_host,
                              uint width, uint height, uint depth);
template double* cosine_transform_kernel_launch(double* image_host,
                              uint width, uint height, uint depth);
