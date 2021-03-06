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

#ifndef ITKIMAGE_H
#define ITKIMAGE_H

#include <itkImage.h>

#include <string>

#include <QPoint>
#include <QString>

#include "PixelIndex.h"

class ITKImage
{
public:
    typedef unsigned int uint;
    static const uint ImageDimension = 3;
    typedef float PixelType;

    typedef itk::Image<PixelType, ImageDimension> InnerITKImage;
    typedef InnerITKImage::IndexType Index;

    typedef PixelIndex IndexType;

    uint width;
    uint height;
    uint depth;
    uint voxel_count;

    typedef PixelIndex Size;

private:
    InnerITKImage::Pointer inner_image;
public:
    ITKImage();
    ITKImage(const ITKImage&);
    ITKImage& operator=(ITKImage image)
    {
        this->width = image.width;
        this->height = image.height;
        this->depth = image.depth;
        this->voxel_count = image.voxel_count;
        this->inner_image = image.inner_image;
        return *this;
    }

    ITKImage(uint width, uint height, uint depth);
    ITKImage(InnerITKImage::Pointer inner_image);
    ITKImage(uint width, uint height, uint depth, InnerITKImage::PixelType* data);

    InnerITKImage::Pointer getPointer() const;
    ITKImage clone() const;

    static ITKImage read(std::string image_file_path);

    static ITKImage read_hsv(std::string image_file_path);
    void write_hsv(std::string image_file_path) const;

    void write(std::string image_file_path) const;

    bool isNull() const;

    void foreachPixel(std::function<void(uint x, uint y, uint z, PixelType pixel)> callback) const;

    PixelType getPixel(uint x, uint y, uint z) const;
    void setPixel(uint x, uint y, uint z, PixelType value);
    void setPixel(Index index, PixelType value);
    void setPixel(PixelIndex index, PixelType value);
    void setPixel(uint linear_index, PixelType value);

    void setEachPixel(std::function<PixelType(uint x, uint y, uint z)> pixel_fetcher);

    PixelType getPixel(InnerITKImage::IndexType index) const;
    PixelType getPixel(PixelIndex index) const;

    uint getImageDimension() const;
    uint getDepth() const;

    PixelType minimum() const;
    PixelType maximum() const;
    void minimumAndMaximum(PixelType& minimum, PixelType& maximum) const;

    static const ITKImage Null;

    static Index indexFromPoint(QPoint point, uint slice_index);
    static QPoint pointFromIndex(Index index);
    static QString indexToText(Index index);

    uint linearIndex(Index index) const;
    uint linearIndex(uint x, uint y, uint z) const;
    bool contains(Index index) const;
    bool contains(PixelIndex index) const;

    ITKImage::Index linearTo3DIndex(uint linear_index) const;

    PixelType* cloneToPixelArray() const;
    PixelType* cloneToCudaPixelArray() const;
    ITKImage cloneSameSizeWithZeros() const;

    static uint linearIndex(Size size, ITKImage::InnerITKImage::IndexType index);
    static PixelType getPixel(PixelType* image_data, Size size, ITKImage::InnerITKImage::IndexType index);
    static void setPixel(PixelType* image_data, Size size, ITKImage::InnerITKImage::IndexType index, PixelType value);

    void setOriginAndSpacingOf(const ITKImage& source_image);
    bool hasSameSize(const ITKImage& other_image);
    void minimumAndMaximumInsideMask(PixelType &minimum, PixelType &maximum, const ITKImage &mask) const;
private:
    void write_png(std::string image_file_path) const;
    void write_dicom(std::string image_file_path) const;
};

#endif // ITKIMAGE_H
