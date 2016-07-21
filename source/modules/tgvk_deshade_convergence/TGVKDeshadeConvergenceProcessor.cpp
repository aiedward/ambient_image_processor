#include "TGVKDeshadeConvergenceProcessor.h"

#include "CudaImageOperationsProcessor.h"
#include "TGVDeshadeProcessor.h"

template<typename Pixel>
Pixel* tgvk_l1_deshade_convergence_launch(Pixel* f_host,
                              uint width, uint height, uint depth,
                              Pixel lambda,
                              uint iteration_count,
                              uint paint_iteration_interval,
                              TGVDeshadeProcessor::IterationCallback<Pixel> iteration_finished_callback,
                              const uint order,
                              const Pixel* alpha,
                              Pixel** v_x, Pixel**v_y, Pixel**v_z,
                              Pixel* mask_host,
                              Pixel* convergence_curve);

template<typename Pixel>
Pixel* tgv2_l1_deshade_launch(Pixel* f_host,
                              uint width, uint height, uint depth,
                              Pixel lambda,
                              uint iteration_count,
                              uint paint_iteration_interval,
                              TGVDeshadeProcessor::IterationCallback<Pixel> iteration_finished_callback,
                              Pixel alpha0,
                              Pixel alpha1,
                              Pixel** v_x, Pixel**v_y, Pixel**v_z);

TGVKDeshadeConvergenceProcessor::TGVKDeshadeConvergenceProcessor()
{
}

void TGVKDeshadeConvergenceProcessor::processTGVKL1Cuda(ITKImage input_image,
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
                             ITKImage::PixelType* convergence_curve)
{
    Pixel* f = input_image.cloneToPixelArray();

    ITKImage background_mask;
    if(add_background_back && !mask.isNull())
      background_mask = CudaImageOperationsProcessor::invert(mask);

    IterationCallback<Pixel> iteration_callback =
            [add_background_back, &background_mask, &input_image, iteration_finished_callback, &mask,
            set_negative_values_to_zero] (
            uint iteration_index, uint iteration_count, Pixel* u_pixels,
            Pixel* v_x, Pixel* v_y, Pixel* v_z) {
        auto u = ITKImage(input_image.width, input_image.height, input_image.depth, u_pixels);
        auto l = ITKImage();
        auto r = TGVDeshadeProcessor::deshade_poisson_cosine_transform(u, v_x, v_y, v_z,
                                                  input_image.width, input_image.height, input_image.depth,
                                                  mask, set_negative_values_to_zero,
                                                  l);

        if(add_background_back && !mask.isNull())
        {
            auto background = CudaImageOperationsProcessor::multiply(u, background_mask);
            r = CudaImageOperationsProcessor::add(r, background);
        }

        return iteration_finished_callback(iteration_index, iteration_count, u, l, r);
    };

    Pixel* v_x, *v_y, *v_z;
    Pixel* u = nullptr;

    Pixel* mask_pixels = mask.cloneToPixelArray();

    if(order == 2)
    {
        u = tgv2_l1_deshade_launch(f,
                                  input_image.width, input_image.height, input_image.depth,
                                  lambda,
                                  iteration_count,
                                  paint_iteration_interval,
                                  iteration_callback,
                                  alpha[1], alpha[0],
                                  &v_x, &v_y, &v_z);
    } else {
        u = tgvk_l1_deshade_convergence_launch(f,
                                  input_image.width, input_image.height, input_image.depth,
                                  lambda,
                                  iteration_count,
                                  paint_iteration_interval,
                                  iteration_callback,
                                  order, alpha,
                                  &v_x, &v_y, &v_z,
                                  mask_pixels,
                                  convergence_curve);
    }
    delete[] mask_pixels;

    delete[] f;
    denoised_image = ITKImage(input_image.width, input_image.height, input_image.depth, u);
    delete[] u;
    deshaded_image = TGVDeshadeProcessor::deshade_poisson_cosine_transform(denoised_image, v_x, v_y, v_z,
                                                           input_image.width, input_image.height, input_image.depth,
                                                           mask, set_negative_values_to_zero,
                                                           shading_image, true);

    // calculate div v
    Pixel* divergence = CudaImageOperationsProcessor::divergence(v_x, v_y, v_z,
                                                                 input_image.width, input_image.height, input_image.depth,
                                                                 true);
    div_v_image = ITKImage(input_image.width, input_image.height, input_image.depth, divergence);
    delete[] divergence;

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
