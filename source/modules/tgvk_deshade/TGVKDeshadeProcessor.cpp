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

#include "TGVKDeshadeProcessor.h"

#include "CudaImageOperationsProcessor.h"
#include "TGVDeshadeProcessor.h"

template<typename Pixel>
Pixel* tgvk_l1_deshade_launch(Pixel* f_host,
                              uint width, uint height, uint depth,
                              Pixel lambda,
                              uint iteration_count,
                              uint paint_iteration_interval,
                              TGVDeshadeProcessor::IterationCallback<Pixel> iteration_finished_callback,
                              const uint order,
                              const Pixel* alpha,
                              Pixel** v_x, Pixel**v_y, Pixel**v_z);

template<typename Pixel>
Pixel* tgv2_l1_deshade_launch(Pixel* f_host,
                              uint width, uint height, uint depth,
                              Pixel lambda,
                              uint iteration_count,
                              uint paint_iteration_interval,
                              const int cuda_block_dimension,
                              TGVDeshadeProcessor::IterationCallback<Pixel> iteration_finished_callback,
                              Pixel alpha0,
                              Pixel alpha1,
                              Pixel** v_x, Pixel**v_y, Pixel**v_z);

template<typename Pixel>
Pixel* tgvk_l1_deshade_launch_2d(Pixel* f_host,
                              uint width, uint height,
                              Pixel lambda,
                              uint iteration_count,
                              uint paint_iteration_interval,
                              TGVDeshadeProcessor::IterationCallback2D<Pixel> iteration_finished_callback,
                              const uint order,
                              const Pixel* alpha,
                              Pixel** v_x, Pixel**v_y);

template<typename Pixel>
Pixel* tgv2_l1_deshade_launch_2d(Pixel* f_host,
                              uint width, uint height,
                              Pixel lambda,
                              uint iteration_count,
                              uint paint_iteration_interval,
                              TGVDeshadeProcessor::IterationCallback2D<Pixel> iteration_finished_callback,
                              Pixel alpha0,
                              Pixel alpha1,
                              Pixel** v_x, Pixel**v_y);

TGVKDeshadeProcessor::TGVKDeshadeProcessor()
{
}

void TGVKDeshadeProcessor::processTGVKL1Cuda(ITKImage input_image,
                             const Pixel lambda,

                             const uint order,
                             const Pixel* alpha,

                             const uint iteration_count,
                             const ITKImage& mask,
                             const bool set_negative_values_to_zero,
                             const bool add_background_back,

                             const uint paint_iteration_interval,
                             IterationFinishedThreeImages iteration_finished_callback,

                             ITKImage& denoised_image,
                             ITKImage& shading_image,
                             ITKImage& deshaded_image,
                             ITKImage& div_v_image,
                             const bool calculate_div_v)
{
    if(input_image.depth == 1)
    {
        processTGVKL1Cuda2D(input_image, lambda, order, alpha, iteration_count, mask, set_negative_values_to_zero,
                            add_background_back, paint_iteration_interval, iteration_finished_callback,
                            denoised_image, shading_image, deshaded_image, div_v_image, calculate_div_v);
        return;
    }

    Pixel* f = input_image.cloneToPixelArray();

    ITKImage background_mask;
    if(add_background_back && !mask.isNull())
      background_mask = CudaImageOperationsProcessor::invert(mask);

    IterationCallback<Pixel> iteration_callback = nullptr;

    if(iteration_finished_callback != nullptr)
        iteration_callback =
        [add_background_back, &background_mask, &input_image, iteration_finished_callback, &mask,
        set_negative_values_to_zero] (
        uint iteration_index, uint iteration_count, Pixel* u_pixels,
        Pixel* v_x, Pixel* v_y, Pixel* v_z) {
            auto u = ITKImage(input_image.width, input_image.height, input_image.depth, u_pixels);
            auto l = ITKImage();
            auto r = TGVDeshadeProcessor::deshade_poisson_cosine_transform(u, v_x, v_y, v_z,
                                                      input_image.width, input_image.height, input_image.depth,
                                                      mask, set_negative_values_to_zero,
                                                      l, true);

            if(add_background_back && !mask.isNull())
            {
                auto background = CudaImageOperationsProcessor::multiply(u, background_mask);
                r = CudaImageOperationsProcessor::add(r, background);
            }

            return iteration_finished_callback(iteration_index, iteration_count, u, l, r);
    };

    Pixel* v_x, *v_y, *v_z;
    Pixel* u = nullptr;

    if(order == 2)
    {
        u = tgv2_l1_deshade_launch(f,
                                  input_image.width, input_image.height, input_image.depth,
                                  lambda,
                                  iteration_count,
                                  paint_iteration_interval,
                                  -1,
                                  iteration_callback,
                                  alpha[1], alpha[0],
                                  &v_x, &v_y, &v_z);
    } else {
        u = tgvk_l1_deshade_launch(f,
                                  input_image.width, input_image.height, input_image.depth,
                                  lambda,
                                  iteration_count,
                                  paint_iteration_interval,
                                  iteration_callback,
                                  order, alpha,
                                  &v_x, &v_y, &v_z);
    }

    delete[] f;
    denoised_image = ITKImage(input_image.width, input_image.height, input_image.depth, u);
    delete[] u;
    deshaded_image = TGVDeshadeProcessor::deshade_poisson_cosine_transform(denoised_image, v_x, v_y, v_z,
                                                           input_image.width, input_image.height, input_image.depth,
                                                           mask, set_negative_values_to_zero,
                                                           shading_image, true);

    if(calculate_div_v)
    {
        Pixel* divergence = CudaImageOperationsProcessor::divergence(v_x, v_y, v_z,
                                                                     input_image.width, input_image.height, input_image.depth,
                                                                     true);
        div_v_image = ITKImage(input_image.width, input_image.height, input_image.depth, divergence);
        delete[] divergence;
    }

    delete[] v_x;
    delete[] v_y;
    if(input_image.depth > 1)
        delete[] v_z;

    if(add_background_back && !mask.isNull())
    {
        auto background = CudaImageOperationsProcessor::multiply(denoised_image, background_mask);
        deshaded_image = CudaImageOperationsProcessor::add(deshaded_image, background);
    }
}


void TGVKDeshadeProcessor::processTGVKL1Cuda2D(ITKImage input_image,
                             const Pixel lambda,

                             const uint order,
                             const Pixel* alpha,

                             const uint iteration_count,
                             const ITKImage& mask,
                             const bool set_negative_values_to_zero,
                             const bool add_background_back,

                             const uint paint_iteration_interval,
                             IterationFinishedThreeImages iteration_finished_callback,

                             ITKImage& denoised_image,
                             ITKImage& shading_image,
                             ITKImage& deshaded_image,
                             ITKImage& div_v_image,
                             const bool calculate_div_v)
{
    Pixel* f = input_image.cloneToPixelArray();

    ITKImage background_mask;
    if(add_background_back && !mask.isNull())
      background_mask = CudaImageOperationsProcessor::invert(mask);

    IterationCallback2D<Pixel> iteration_callback = nullptr;

    if(iteration_finished_callback != nullptr)
        iteration_callback =
        [add_background_back, &background_mask, &input_image, iteration_finished_callback, &mask,
        set_negative_values_to_zero] (
        uint iteration_index, uint iteration_count, Pixel* u_pixels,
        Pixel* v_x, Pixel* v_y) {
            auto u = ITKImage(input_image.width, input_image.height, input_image.depth, u_pixels);
            auto l = ITKImage();
            auto r = TGVDeshadeProcessor::deshade_poisson_cosine_transform_2d(u, v_x, v_y,
                                                      input_image.width, input_image.height,
                                                      mask, set_negative_values_to_zero,
                                                      l, true);

            if(add_background_back && !mask.isNull())
            {
                auto background = CudaImageOperationsProcessor::multiply(u, background_mask);
                r = CudaImageOperationsProcessor::add(r, background);
            }

            return iteration_finished_callback(iteration_index, iteration_count, u, l, r);
    };

    Pixel* v_x, *v_y;
    Pixel* u = nullptr;

    if(order == 2)
    {
        u = tgv2_l1_deshade_launch_2d(f,
                                  input_image.width, input_image.height,
                                  lambda,
                                  iteration_count,
                                  paint_iteration_interval,
                                  iteration_callback,
                                  alpha[1], alpha[0],
                                  &v_x, &v_y);
    } else {
        u = tgvk_l1_deshade_launch_2d(f,
                              input_image.width, input_image.height,
                              lambda,
                              iteration_count,
                              paint_iteration_interval,
                              iteration_callback,
                              order, alpha,
                              &v_x, &v_y);
    }

    delete[] f;
    denoised_image = ITKImage(input_image.width, input_image.height, input_image.depth, u);
    delete[] u;
    deshaded_image = TGVDeshadeProcessor::deshade_poisson_cosine_transform_2d(denoised_image, v_x, v_y,
                                                           input_image.width, input_image.height,
                                                           mask, set_negative_values_to_zero,
                                                           shading_image, true);

    if(calculate_div_v)
    {
        Pixel* divergence = CudaImageOperationsProcessor::divergence_2d(v_x, v_y,
                                                                     input_image.width, input_image.height,
                                                                     true);
        div_v_image = ITKImage(input_image.width, input_image.height, input_image.depth, divergence);
        delete[] divergence;
    }

    delete[] v_x;
    delete[] v_y;

    if(add_background_back && !mask.isNull())
    {
        auto background = CudaImageOperationsProcessor::multiply(denoised_image, background_mask);
        deshaded_image = CudaImageOperationsProcessor::add(deshaded_image, background);
    }
}

void TGVKDeshadeProcessor::updateAlpha(uint order,
                                              std::function<void(uint alpha_element)> updateElement)
{
    const uint increment = 3; // alpha: 1, 4, 7...

    for(int i = 0; i < order; i++)
    {
        updateElement(1 + i * increment);
    }
}
