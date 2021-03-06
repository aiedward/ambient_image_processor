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

#include "CudaImageOperationsProcessor.h"

//#include <fftw3.h> this was used as a reference implementation

template<typename Pixel>
Pixel* multiply_kernel_launch(Pixel* image1, Pixel* image2,
                              uint width, uint height, uint depth);
template<typename Pixel>
Pixel* divide_kernel_launch(Pixel* image1, Pixel* image2,
                            uint width, uint height, uint depth);
template<typename Pixel>
Pixel* add_kernel_launch(Pixel* image1, Pixel* image2,
                         uint width, uint height, uint depth);
template<typename Pixel>
Pixel* subtract_kernel_launch(Pixel* image1, Pixel* image2,
                              uint width, uint height, uint depth);
template<typename Pixel>
Pixel* convolution3x3_kernel_launch(Pixel* image1,
                                    uint width, uint height,
                                    Pixel* kernel, bool calculate_center_as_sum_of_others);
template<typename Pixel>
Pixel* convolution3x3x3_kernel_launch(Pixel* image1,
                                      uint width, uint height, uint depth,
                                      Pixel* kernel, bool calculate_center_as_sum_of_others);

template<typename Pixel>
Pixel* add_constant_kernel_launch(Pixel* image1,
                                  uint width, uint height, uint depth,
                                  Pixel constant);
template<typename Pixel>
Pixel* multiply_constant_kernel_launch(Pixel* image1,
                                       uint width, uint height, uint depth,
                                       Pixel constant);

template<typename Pixel>
Pixel* cosine_transform_kernel_launch(Pixel* image,
                                      uint width, uint height, uint depth);
template<typename Pixel>
Pixel* inverse_cosine_transform_kernel_launch(Pixel* image,
                                              uint width, uint height, uint depth);

template<typename Pixel>
Pixel* divergence_kernel_launch(
        Pixel* dx, Pixel* dy, Pixel* dz,
        const uint width, const uint height, const uint depth, bool is_host_data=false);

template<typename Pixel>
Pixel* divergence_2d_kernel_launch(
        Pixel* dx, Pixel* dy,
        const uint width, const uint height, bool is_host_data=false);

template<typename Pixel>
Pixel* solve_poisson_in_cosine_domain_kernel_launch(Pixel* image_host,
                                                    uint width, uint height, uint depth);

template<typename Pixel>
Pixel* invert_kernel_launch(Pixel* image,
                            uint width, uint height, uint depth);
template<typename Pixel>
Pixel* binary_dilate_kernel_launch(Pixel* image,
                                   uint width, uint height, uint depth);

template<typename Pixel>
Pixel* clamp_negative_values_kernel_launch(Pixel* image,
                                           uint width, uint height, uint depth, Pixel value);
template<typename Pixel>
Pixel* binarize_kernel_launch(Pixel* image,
                              uint width, uint height, uint depth);


template<typename Pixel>
double tv_kernel_launch(Pixel* image, uint width, uint height, uint depth);

template<typename Pixel>
Pixel* log_kernel_launch(Pixel* image, uint width, uint height, uint depth);
template<typename Pixel>
Pixel* exp_kernel_launch(Pixel* image, uint width, uint height, uint depth);

template<typename Pixel>
Pixel* div_grad_kernel_launch(Pixel* image, uint width, uint height, uint depth);
template<typename Pixel>
Pixel* div_grad_2d_kernel_launch(Pixel* image, uint width, uint height);

template<typename Pixel>
Pixel* rotate_180_in_plane_kernel_launch(Pixel* image, uint width, uint height, uint depth);

CudaImageOperationsProcessor::CudaImageOperationsProcessor()
{
}

ITKImage CudaImageOperationsProcessor::multiply(ITKImage image1, ITKImage image2)
{
    return perform(image1, image2, [&image1](Pixels pixels1, Pixels pixels2) {
        return multiply_kernel_launch(pixels1, pixels2,
                                      image1.width,
                                      image1.height,
                                      image1.depth);
    });
}

ITKImage CudaImageOperationsProcessor::divide(ITKImage image1, ITKImage image2)
{
    return perform(image1, image2, [&image1](Pixels pixels1, Pixels pixels2) {
        return divide_kernel_launch(pixels1, pixels2,
                                    image1.width,
                                    image1.height,
                                    image1.depth);
    });
}

ITKImage CudaImageOperationsProcessor::add(ITKImage image1, ITKImage image2)
{
    return perform(image1, image2, [&image1](Pixels pixels1, Pixels pixels2) {
        return add_kernel_launch(pixels1, pixels2,
                                 image1.width,
                                 image1.height,
                                 image1.depth);
    });
}

ITKImage CudaImageOperationsProcessor::subtract(ITKImage image1, ITKImage image2)
{
    return perform(image1, image2, [&image1](Pixels pixels1, Pixels pixels2) {
        return subtract_kernel_launch(pixels1, pixels2,
                                      image1.width,
                                      image1.height,
                                      image1.depth);
    });
}

ITKImage CudaImageOperationsProcessor::perform(ITKImage image1, ITKImage image2, BinaryPixelsOperation operation)
{
    auto image1_pixels = image1.cloneToPixelArray();
    auto image2_pixels = image2.cloneToPixelArray();

    auto result_pixels = operation(image1_pixels, image2_pixels);
    auto result = ITKImage(image1.width, image1.height, image1.depth, result_pixels);

    delete[] image1_pixels;
    delete[] image2_pixels;
    delete[] result_pixels;

    result.setOriginAndSpacingOf(image1);
    return result;
}

ITKImage CudaImageOperationsProcessor::perform(ITKImage image, UnaryPixelsOperation operation)
{
    auto image_pixels = image.cloneToPixelArray();

    auto result_pixels = operation(image_pixels);

    auto result = ITKImage(image.width, image.height, image.depth, result_pixels);
    delete[] result_pixels;
    delete[] image_pixels;

    return result;
}

ITKImage CudaImageOperationsProcessor::addConstant(ITKImage image, ITKImage::PixelType constant)
{
    return perform(image, [&image, constant](Pixels image_pixels) {
        return add_constant_kernel_launch(image_pixels,
                                          image.width, image.height, image.depth, constant);
    });
}

ITKImage CudaImageOperationsProcessor::multiplyConstant(ITKImage image, ITKImage::PixelType constant)
{
    return perform(image, [&image, constant](Pixels image_pixels) {
        return multiply_constant_kernel_launch(image_pixels,
                                               image.width, image.height, image.depth, constant);
    });
}

ITKImage CudaImageOperationsProcessor::convolution3x3(ITKImage image, ITKImage::PixelType* kernel,
                                                      bool calculate_center_as_sum_of_others)
{
    return perform(image, [&image, kernel, calculate_center_as_sum_of_others](Pixels image_pixels) {
        return convolution3x3_kernel_launch(image_pixels,
                                            image.width, image.height, kernel, calculate_center_as_sum_of_others);
    });
}

ITKImage CudaImageOperationsProcessor::convolution3x3x3(ITKImage image, ITKImage::PixelType* kernel,
                                                        bool calculate_center_as_sum_of_others)
{
    return perform(image, [&image, kernel, calculate_center_as_sum_of_others](
                   Pixels image_pixels) {
        return convolution3x3x3_kernel_launch(image_pixels,
                                              image.width, image.height, image.depth, kernel,
                                              calculate_center_as_sum_of_others);
    });
}

ITKImage CudaImageOperationsProcessor::divGrad(ITKImage image) {
    return perform(image, [&image](Pixels image_pixels) {
        if(image.depth == 1)
            return div_grad_2d_kernel_launch(image_pixels, image.width, image.height);

        return div_grad_kernel_launch(image_pixels, image.width, image.height, image.depth);
    });
}

ITKImage CudaImageOperationsProcessor::remove_zero_frequency(ITKImage image)
{
    auto dct_image = cosineTransform(image);
    dct_image.setPixel(0,0,0, 0);
    return inverseCosineTransform(dct_image);
}

ITKImage CudaImageOperationsProcessor::rotate180InPlane(ITKImage image)
{
    return perform(image, [&image](Pixels image_pixels) {
        return rotate_180_in_plane_kernel_launch(image_pixels, image.width, image.height, image.depth);
    });
}

ITKImage CudaImageOperationsProcessor::cosineTransform(ITKImage image)
{
    return perform(image, [&image](Pixels image_pixels) {
        return cosine_transform_kernel_launch(image_pixels, image.width, image.height, image.depth);

        /* reference implementation:
        Pixels result = new Pixel[image.width * image.height * image.depth];
        fftw_plan plan = fftw_plan_r2r_3d((int)image.depth, (int) image.height, (int) image.width,
                                   image_pixels, result,
                                   FFTW_REDFT10, FFTW_REDFT10, FFTW_REDFT10,
                                   FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
        fftw_execute(plan);

        fftw_destroy_plan(plan);
        fftw_cleanup();

        return result;
        */
    });
}

ITKImage CudaImageOperationsProcessor::inverseCosineTransform(ITKImage image)
{
    return perform(image, [&image](Pixels image_pixels) {
        return inverse_cosine_transform_kernel_launch(image_pixels, image.width, image.height, image.depth);

        /* reference implementation
        Pixels result = new Pixel[image.width * image.height * image.depth];
        fftw_plan plan = fftw_plan_r2r_3d((int)image.depth, (int) image.height, (int) image.width,
                                   image_pixels, result,
                                   FFTW_REDFT01, FFTW_REDFT01, FFTW_REDFT01,
                                   FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
        fftw_execute(plan);

        fftw_destroy_plan(plan);
        fftw_cleanup();

        Pixel constant = 0.25 * 0.5 / image.voxel_count;
        result = multiply_constant_kernel_launch(result,
                                                 image.width, image.height, image.depth, constant);

        return result;
        */
    });
}

ITKImage::PixelType* CudaImageOperationsProcessor::divergence(
        ITKImage::PixelType* dx, ITKImage::PixelType* dy, ITKImage::PixelType* dz,
        const uint width, const uint height, const uint depth, bool is_host_data)
{
    if(depth == 1)
        return divergence_2d(dx, dy, width, height, is_host_data);

    return divergence_kernel_launch(dx, dy, dz, width, height, depth, is_host_data);
}

ITKImage::PixelType* CudaImageOperationsProcessor::divergence_2d(
        ITKImage::PixelType* dx, ITKImage::PixelType* dy,
        const uint width, const uint height, bool is_host_data)
{
    return divergence_2d_kernel_launch(dx, dy, width, height, is_host_data);
}

ITKImage CudaImageOperationsProcessor::solvePoissonInCosineDomain(ITKImage image)
{
    return perform(image, [&image](Pixels image_pixels) {
        return solve_poisson_in_cosine_domain_kernel_launch(image_pixels,
                                                            image.width, image.height, image.depth);
    });
}

ITKImage CudaImageOperationsProcessor::invert(ITKImage image)
{
    return perform(image, [&image](Pixels image_pixels) {
        return invert_kernel_launch(image_pixels,
                                    image.width, image.height, image.depth);
    });
}


ITKImage CudaImageOperationsProcessor::log(ITKImage image)
{
    return perform(image, [&image](Pixels image_pixels) {
        return log_kernel_launch(image_pixels,
                                 image.width, image.height, image.depth);
    });
}
ITKImage CudaImageOperationsProcessor::exp(ITKImage image)
{
    return perform(image, [&image](Pixels image_pixels) {
        return exp_kernel_launch(image_pixels,
                                 image.width, image.height, image.depth);
    });
}

ITKImage CudaImageOperationsProcessor::binary_dilate(ITKImage image)
{
    /*
    auto result = image.clone();
    image.foreachPixel([&result](uint x, uint y, uint z, Pixel value) {
        auto center = PixelIndex(x,y,z);
        if(value < 0.5)
        {
            result.setPixel(center, 0);
            return;
        }

        auto indices = center.collectNeighboursInSlice(PixelIndex(result.width, result.height, result.depth));
        for(auto index : indices)
            result.setPixel(index, 1);
    });
    return result;
    */
    return perform(image, [&image](Pixels image_pixels) {
        return binary_dilate_kernel_launch(image_pixels,
                                           image.width, image.height, image.depth);
    });
}

ITKImage CudaImageOperationsProcessor::clamp_negative_values(ITKImage image, ITKImage::PixelType value)
{
    return perform(image, [&image, value](Pixels image_pixels) {
        return clamp_negative_values_kernel_launch(image_pixels,
                                                   image.width, image.height, image.depth, value);
    });
}

double CudaImageOperationsProcessor::tv(ITKImage image)
{
    ITKImage::PixelType* image_pixels = image.cloneToPixelArray();

    auto result = tv_kernel_launch(image_pixels, image.width, image.height, image.depth);

    delete[] image_pixels;

    return result;
}

ITKImage CudaImageOperationsProcessor::binarize(ITKImage image)
{
    return perform(image, [&image](Pixels image_pixels) {
        return binarize_kernel_launch(image_pixels, image.width, image.height, image.depth);
    });
}
