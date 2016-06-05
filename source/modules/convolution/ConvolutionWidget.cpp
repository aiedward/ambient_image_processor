#include "ConvolutionWidget.h"
#include "ui_ConvolutionWidget.h"

#include "CudaImageOperationsProcessor.h"

ConvolutionWidget::ConvolutionWidget(QString title, QWidget *parent) :
    BaseModuleWidget(title, parent),
    ui(new Ui::ConvolutionWidget)
{
    ui->setupUi(this);
    this->on_load_laplace_setting_button_clicked();
}

ConvolutionWidget::~ConvolutionWidget()
{
    delete ui;
}

void ConvolutionWidget::on_k12_valueChanged(double value)
{
    auto text = QString::number(value);
    this->ui->k23->setText(text);
    this->ui->k32->setText(text);
    this->ui->k21->setText(text);
}

void ConvolutionWidget::on_k11_valueChanged(double value)
{
    auto text = QString::number(value);
    this->ui->k13->setText(text);
    this->ui->k31->setText(text);
    this->ui->k33->setText(text);
}

void ConvolutionWidget::on_load_laplace_setting_button_clicked()
{
    this->ui->k22->setValue(6);
    this->ui->k11->setValue(0);
    this->ui->k12->setValue(-1);
}

void ConvolutionWidget::on_perform_button_clicked()
{
    this->processInWorkerThread();
}

ITKImage ConvolutionWidget::processImage(ITKImage image)
{
    const uint kernel_size = 9*3;
    ITKImage::PixelType kernel[kernel_size];

    // first layer...
    kernel[0] = kernel[2] = kernel[6] = kernel[8] =
        kernel[1] = kernel[3] = kernel[5] = kernel[7] = this->ui->k11->value();
    kernel[4] = this->ui->k12->value();
    // center layer...
    kernel[9] = kernel[11] = kernel[15] = kernel[17] = this->ui->k11->value();
    kernel[10] = kernel[12] = kernel[14] = kernel[16] = this->ui->k12->value();
    kernel[13] = this->ui->k22->value();
    // last layer...
    kernel[18] = kernel[20] = kernel[24] = kernel[26] =
        kernel[19] = kernel[21] = kernel[23] = kernel[25] = this->ui->k11->value();
    kernel[22] = this->ui->k12->value();

    // normalize...
    ITKImage::PixelType sum = 0;
    for(int i = 0; i < kernel_size; i++)
        sum+= kernel[i];

    if(std::abs(sum) > 1e-3)
        for(int i = 0; i < kernel_size; i++)
            kernel[i] /= sum;

    bool correct_center = this->ui->correct_center_checkbox->isChecked();
    return CudaImageOperationsProcessor::convolution3x3x3(image, kernel, correct_center);
}

void ConvolutionWidget::on_load_mean_setting_button_clicked()
{
    this->ui->k22->setValue(1);
    this->ui->k11->setValue(1);
    this->ui->k12->setValue(1);
}
