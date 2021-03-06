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

#include "BinaryOperationsWidget.h"
#include "ui_BinaryOperationsWidget.h"

#include <QFileDialog>

#include "CudaImageOperationsProcessor.h"

BinaryOperationsWidget::BinaryOperationsWidget(QString title, QWidget *parent) :
    BaseModuleWidget(title, parent),
    ui(new Ui::BinaryOperationsWidget)
{
    ui->setupUi(this);

    this->second_image_widget = new ImageViewWidget("Second Image View", this->ui->second_image_frame);
    this->ui->second_image_frame->layout()->addWidget(this->second_image_widget);
}

BinaryOperationsWidget::~BinaryOperationsWidget()
{
    delete ui;
}

void BinaryOperationsWidget::on_load_button_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, "open volume file");
    if(file_name == QString::null || !QFile(file_name).exists())
        return;

    this->second_image_widget->setImage(ITKImage::read(file_name.toStdString()));
}

void BinaryOperationsWidget::on_perform_button_clicked()
{
    this->processInWorkerThread();
}

ITKImage BinaryOperationsWidget::processImage(ITKImage image1)
{
    auto image2 = this->second_image_widget->getImage();

    ITKImage::PixelType image2_factor = this->ui->image2_factor_spinbox->value();
    if(!image2.isNull())
        image2 = CudaImageOperationsProcessor::multiplyConstant(image2, image2_factor);
    else
        std::cout << "no second image given: ignoring the configured factor" << std::endl;


    ITKImage::PixelType image2_offset = this->ui->image2_offset_spinbox->value();
    if(!image2.isNull())
        image2 = CudaImageOperationsProcessor::addConstant(image2, image2_offset);
    else {
        std::cout << "no second image given: creating constant image" << std::endl;
        image2 = image1.clone();
        image2.setEachPixel([image2_offset](uint, uint, uint) {
            return image2_offset;
        });
    }

    if(!image1.hasSameSize(image2))
        throw std::runtime_error("second image has different size");

    if(this->ui->divide_checkbox->isChecked())
        return CudaImageOperationsProcessor::divide(image1, image2);
    if(this->ui->multiply_checkbox->isChecked())
        return CudaImageOperationsProcessor::multiply(image1, image2);
    if(this->ui->add_checkbox->isChecked())
        return CudaImageOperationsProcessor::add(image1, image2);
    if(this->ui->subtract_checkbox->isChecked())
        return CudaImageOperationsProcessor::subtract(image1, image2);

    return ITKImage();
}

void BinaryOperationsWidget::registerModule(ImageWidget *image_widget)
{
    BaseModuleWidget::registerModule(image_widget);

    connect(image_widget, &ImageWidget::sliceIndexChanged,
            this->second_image_widget, &ImageViewWidget::sliceIndexChanged);

    this->second_image_widget->registerCrosshairSubmodule(image_widget);
}

void BinaryOperationsWidget::on_image2_offset_spinbox_valueChanged(double )
{
    this->processInWorkerThread();
}

void BinaryOperationsWidget::on_image2_factor_spinbox_valueChanged(double arg1)
{
    this->processInWorkerThread();
}

void BinaryOperationsWidget::on_clearButton_clicked()
{
    this->second_image_widget->setImage(ITKImage());
}
