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

#include "TGV3DeshadeWidget.h"
#include "ui_TGV3DeshadeWidget.h"

#include <QFileDialog>

TGV3DeshadeWidget::TGV3DeshadeWidget(QString title, QWidget *parent) :
    BaseModuleWidget(title, parent),
    ui(new Ui::TGV3DeshadeWidget)
{
    ui->setupUi(this);

    this->shading_output_view = new ImageViewWidget("Denoised", this->ui->shading_frame);
    this->ui->shading_frame->layout()->addWidget(this->shading_output_view);

    this->denoised_output_view = new ImageViewWidget("Denoised", this->ui->denoised_frame);
    this->ui->denoised_frame->layout()->addWidget(this->denoised_output_view);

    this->mask_view = new ImageViewWidget("Mask", this->ui->mask_frame);
    this->ui->mask_frame->layout()->addWidget(this->mask_view);
}

TGV3DeshadeWidget::~TGV3DeshadeWidget()
{
    delete ui;
}


void TGV3DeshadeWidget::registerModule(ImageWidget *image_widget)
{
    BaseModuleWidget::registerModule(image_widget);

    connect(this, &BaseModuleWidget::fireWorkerFinished,
            this, [this]() {
        this->ui->stop_button->setEnabled(false);
    });

    this->shading_output_view->registerCrosshairSubmodule(image_widget);
    connect(image_widget, &ImageWidget::sliceIndexChanged,
            this->shading_output_view, &ImageViewWidget::sliceIndexChanged);
    this->denoised_output_view->registerCrosshairSubmodule(image_widget);
    connect(image_widget, &ImageWidget::sliceIndexChanged,
            this->denoised_output_view, &ImageViewWidget::sliceIndexChanged);
}

void TGV3DeshadeWidget::setIterationFinishedCallback(TGV3DeshadeProcessor::IterationFinished iteration_finished_callback)
{
    this->iteration_finished_callback = [this, iteration_finished_callback](uint iteration_index, uint iteration_count,
            ITKImage u, ITKImage l, ITKImage r){
        iteration_finished_callback(iteration_index, iteration_count, r);

        this->shading_output_view->fireImageChange(l);
        this->denoised_output_view->fireImageChange(u);

        return this->stop_after_next_iteration;
    };
}

ITKImage TGV3DeshadeWidget::processImage(ITKImage image)
{
    const float alpha0 = this->ui->alpha0_spinbox->value();
    const float alpha1 = this->ui->alpha1_spinbox->value();
    const float alpha2 = this->ui->alpha2_spinbox->value();
    const float lambda = this->ui->lambda_spinbox->value();
    const uint iteration_count = this->ui->iteration_count_spinbox->value();
    const uint paint_iteration_interval = this->ui->paint_iteration_interval_spinbox->value();

    const bool set_negative_values_to_zero = this->ui->set_negative_values_to_zero_checkbox->isChecked();
    auto mask = this->mask_view->getImage();

    const bool add_background_back = this->ui->add_background_back_checkbox->isChecked();

    ITKImage denoised_image = ITKImage();
    ITKImage shading_image = ITKImage();
    ITKImage deshaded_image = ITKImage();

    if(image.depth > 1)
    {
        TGV3DeshadeProcessor::processTGV3L1Cuda(
                  image,
                  lambda,
                  alpha0,
                  alpha1,
                  alpha2,
                  iteration_count,
                  mask,
                  set_negative_values_to_zero,
                  add_background_back,

                  paint_iteration_interval,
                  this->iteration_finished_callback,

                  denoised_image,
                  shading_image,
                  deshaded_image);
    }
    else
    {
        TGV3DeshadeProcessor::processTGV3L1Cuda2D(
                  image,
                  lambda,
                  alpha0,
                  alpha1,
                  alpha2,
                  iteration_count,
                  mask,
                  set_negative_values_to_zero,
                  add_background_back,

                  paint_iteration_interval,
                  this->iteration_finished_callback,

                  denoised_image,
                  shading_image,
                  deshaded_image);
    }

    this->denoised_output_view->setImage(denoised_image);
    this->shading_output_view->setImage(shading_image);
    return deshaded_image;
}


void TGV3DeshadeWidget::on_perform_button_clicked()
{
    this->stop_after_next_iteration = false;
    this->processInWorkerThread();
    this->ui->stop_button->setEnabled(true);
}

void TGV3DeshadeWidget::on_stop_button_clicked()
{
    this->stop_after_next_iteration = true;
}

void TGV3DeshadeWidget::on_save_second_output_button_clicked()
{
    auto image = this->shading_output_view->getImage();
    if(image.isNull())
        return;

    QString file_name = QFileDialog::getSaveFileName(this, "save volume file");
    if(file_name.isNull())
        return;

    image.write(file_name.toStdString());
}

void TGV3DeshadeWidget::on_load_mask_button_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, "open volume file");
    if(file_name == QString::null || !QFile(file_name).exists())
        return;

    this->mask_view->setImage(ITKImage::read(file_name.toStdString()));
}

void TGV3DeshadeWidget::on_save_denoised_button_clicked()
{
    auto image = this->denoised_output_view->getImage();
    if(image.isNull())
        return;

    QString file_name = QFileDialog::getSaveFileName(this, "save volume file");
    if(file_name.isNull())
        return;

    image.write(file_name.toStdString());
}

void TGV3DeshadeWidget::on_clear_mask_button_clicked()
{
    this->mask_view->setImage(ITKImage());
}
