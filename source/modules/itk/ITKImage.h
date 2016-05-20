#ifndef ITKIMAGE_H
#define ITKIMAGE_H

#include <itkImage.h>
#include <string>

#include <QPoint>
#include <QString>

class ITKImage
{
public:
    typedef unsigned int uint;
    static const uint ImageDimension = 3;
    typedef double PixelType;

    typedef itk::Image<PixelType, ImageDimension> InnerITKImage;
    typedef InnerITKImage::IndexType Index;

    uint width;
    uint height;
    uint depth;
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
        this->inner_image = image.inner_image;
        return *this;
    }

    ITKImage(uint width, uint height, uint depth);
    ITKImage(InnerITKImage::Pointer inner_image);
    ITKImage(uint width, uint height, uint depth, InnerITKImage::PixelType* data);

    InnerITKImage::Pointer getPointer() const;
    ITKImage clone() const;

    static ITKImage read(std::string image_file_path);
    void write(std::string image_file_path);

    bool isNull() const;

    void foreachPixel(std::function<void(uint x, uint y, uint z, PixelType pixel)> callback) const;

    PixelType getPixel(uint x, uint y, uint z) const;
    void setPixel(uint x, uint y, uint z, PixelType value);

    void setEachPixel(std::function<PixelType(uint x, uint y, uint z)> pixel_fetcher);

    PixelType getPixel(InnerITKImage::IndexType index) const;

    uint getImageDimension() const;
    uint getDepth() const;

    PixelType minimum() const;
    PixelType maximum() const;

    static ITKImage Null;

    static Index indexFromPoint(QPoint point, uint slice_index);
    static QPoint pointFromIndex(Index index);
    static QString indexToText(Index index);

    uint linearIndex(uint x, uint y, uint z) const;
    bool contains(Index index) const;

    PixelType* cloneToPixelArray() const;
};

#endif // ITKIMAGE_H
