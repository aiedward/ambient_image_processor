#include "BilateralFilterProcessor.h"

#include <itkBilateralImageFilter.h>

template<typename Pixel>
Pixel* bilateral_filter_kernel_launch(
        Pixel* source, uint source_width, uint source_height, uint source_depth,
        uint kernel_size, Pixel sigma_spatial_distance,
        Pixel sigma_intensity_distance);

BilateralFilterProcessor::BilateralFilterProcessor()
{
}

ITKImage BilateralFilterProcessor::process(ITKImage source,
                                          ImageType::PixelType sigma_spatial_distance,
                                          ImageType::PixelType sigma_intensity_distance,
                                          int kernel_size)
{
    ITKImage::PixelType* source_data = source.cloneToPixelArray();

    ITKImage::PixelType* destination_data = bilateral_filter_kernel_launch(source_data,
         source.width, source.height, source.depth, kernel_size,
         sigma_spatial_distance, sigma_intensity_distance);

    delete[] source_data;

    ITKImage destination = ITKImage(source.width, source.height, source.depth, destination_data);

    delete[] destination_data;

    return destination;

    /* ITK :
    typedef itk::BilateralImageFilter<ImageType,ImageType> BilateralFilter;
    BilateralFilter::Pointer filter = BilateralFilter::New();
    filter->SetInput(image.getPointer());
    filter->SetDomainSigma(sigma_intensity_distance);
    filter->SetRangeSigma(sigma_spatial_distance);
    filter->SetAutomaticKernelSize(false);
    BilateralFilter::SizeType radius;
    radius.Fill(kernel_size);
    filter->SetRadius(radius);

    filter->Update();
    ImageType::Pointer output = filter->GetOutput();
    output->DisconnectPipeline();

    return ITKImage(output);
    */

    /*
     *default values:
          this->m_Radius.Fill(1);
          this->m_AutomaticKernelSize = true;
          this->m_DomainSigma.Fill(4.0);
          this->m_RangeSigma = 50.0;
          this->m_FilterDimensionality = ImageDimension;
          this->m_NumberOfRangeGaussianSamples = 100;
          this->m_DynamicRange = 0.0;
          this->m_DynamicRangeUsed = 0.0;
          this->m_DomainMu = 2.5;  // keep small to keep kernels small
          this->m_RangeMu = 4.0;
    */
}
