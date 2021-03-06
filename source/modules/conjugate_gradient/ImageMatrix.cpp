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

#include "ImageMatrix.h"

#include "cuda_helper.cuh"

typedef const unsigned int Dimension;

// kernel launch forward declarations...
template<typename Pixel>
void set_zeros_kernel_launch(Pixel* elements, const unsigned int element_count,
                             dim3 block_dimension, dim3 grid_dimension);
template<typename Pixel>
void transposed_kernel_launch(Pixel* elements, Dimension element_count,
                              Dimension voxel_count,
                              Pixel* transposed_elements,
                              dim3 block_dimension, dim3 grid_dimension);

template<typename Pixel>
void add_kernel_launch(Pixel* elements, Dimension element_count,
                       Pixel* elements2,
                       Pixel* result,
                       dim3 block_dimension, dim3 grid_dimension);

template<typename Pixel>
void multiply_kernel_launch(Pixel* elements, Dimension element_count,
                            Dimension voxel_count,
                            Pixel* elements2,
                            Pixel* result,
                            dim3 block_dimension, dim3 grid_dimension);


template<typename Pixel>
ImageMatrix<Pixel>::ImageMatrix(Dimension voxel_count)
    :voxel_count(voxel_count), element_count(voxel_count*voxel_count)
{
    size_t size = sizeof(Pixel) * element_count;
    cudaCheckError( cudaMalloc(&this->elements, size) )

            this->block_dimension = dim3(CUDA_BLOCK_DIMENSON);
    this->grid_dimension = dim3((element_count + block_dimension.x - 1) / block_dimension.x);
}

template<typename Pixel>
ImageMatrix<Pixel>::~ImageMatrix()
{
    cudaCheckError( cudaFree(this->elements) )
}

template<typename Pixel>
void ImageMatrix<Pixel>::setPixelTransformation(Dimension source_pixel_index,
                                                Dimension target_pixel_index,
                                                Pixel factor)
{
    this->elements[voxel_count * target_pixel_index + source_pixel_index] = factor;
}

template<typename Pixel>
void ImageMatrix<Pixel>::setZeros()
{
    set_zeros_kernel_launch(elements, element_count, block_dimension, grid_dimension);
}

template<typename Pixel>
void ImageMatrix<Pixel>::transposed(ImageMatrix<Pixel>* transposed_matrix)
{
    transposed_kernel_launch(elements, element_count, voxel_count,
                             transposed_matrix->elements,
                             block_dimension, grid_dimension);
}

template<typename Pixel>
void ImageMatrix<Pixel>::add(ImageMatrix<Pixel>* matrix2, ImageMatrix<Pixel>* result)
{
    add_kernel_launch(elements, element_count,
                      matrix2->elements,
                      result->elements,
                      block_dimension, grid_dimension);
}

template<typename Pixel>
void ImageMatrix<Pixel>::multiply(ImageMatrix<Pixel>* matrix2, ImageMatrix<Pixel>* result)
{
    multiply_kernel_launch(elements, element_count, voxel_count,
                           matrix2->elements,
                           result->elements,
                           block_dimension, grid_dimension);
}

template class ImageMatrix<double>;
template class ImageMatrix<float>;
