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

#include "ImageInformationWidget.h"
#include "ui_ImageInformationWidget.h"

#include "ImageWidget.h"
#include "ImageInformationProcessor.h"

#include "CudaImageOperationsProcessor.h"

#include <QClipboard>

ImageInformationWidget::ImageInformationWidget(QString title, QWidget *parent) :
    BaseModuleWidget(title, parent),
    ui(new Ui::ImageInformationWidget)
{
    ui->setupUi(this);
}

ImageInformationWidget::~ImageInformationWidget()
{
    delete ui;
}

void ImageInformationWidget::collectInformation(ITKImage image)
{
    auto information = ImageInformationProcessor::collectInformation(image);

    this->ui->dimensions_label->setText(information["dimensions"]);
    this->ui->voxel_count_label->setText(information["voxel_count"]);
    this->ui->mean_label->setText(information["mean"]);
    this->ui->standard_deviation_label->setText(information["standard_deviation"]);
    this->ui->variance_label->setText(information["variance"]);
    this->ui->coefficient_of_variation->setText(information["coefficient_of_variation"]);
    this->ui->minimum_label->setText(information["minimum"]);
    this->ui->maximum_label->setText(information["maximum"]);
    this->ui->origin_label->setText(information["origin"]);
    this->ui->spacing_label->setText(information["spacing"]);
    this->ui->sum_of_absolute_values_label->setText(information["sum_of_absolute_values"]);
    this->ui->sum_of_squared_values_label->setText(information["sum_of_squared_values"]);

    if(image.isNull())
        return;

    auto tv = CudaImageOperationsProcessor::tv(image) / image.voxel_count;
    this->ui->tv_per_voxel_label->setText(QString::number(tv));
}

void ImageInformationWidget::registerModule(ImageWidget* image_widget)
{
    connect(image_widget, &ImageWidget::imageChanged,
            this, &ImageInformationWidget::collectInformation);
}

void ImageInformationWidget::getCVAndTV(ITKImage::PixelType& cv, ITKImage::PixelType& tv)
{
    cv = this->ui->coefficient_of_variation->text().toDouble();
    tv = this->ui->tv_per_voxel_label->text().toDouble();
}

void ImageInformationWidget::on_copy_to_clipboard_button_clicked()
{
    auto text =
            this->ui->coefficient_of_variation_label->text() + " " +
            this->ui->coefficient_of_variation->text() + "\n" +
            this->ui->tv_per_voxel_text_label->text() + " " +
            this->ui->tv_per_voxel_label->text() + "\n" +
            this->ui->sum_of_absolute_values->text() + " " +
            this->ui->sum_of_absolute_values_label->text() + "\n" +
            this->ui->sum_of_squared_values->text() + " " +
            this->ui->sum_of_squared_values_label->text();

    QApplication::clipboard()->setText(text);
}
