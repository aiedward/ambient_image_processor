#include "TGVWidget.h"
#include "ui_TGVWidget.h"

TGVWidget::TGVWidget(QString title, QWidget *parent) :
    BaseModuleWidget(title, parent),
    ui(new Ui::TGVWidget)
{
    ui->setupUi(this);
}

TGVWidget::~TGVWidget()
{
    delete ui;
}

void TGVWidget::setIterationFinishedCallback(TGVProcessor::IterationFinished iteration_finished_callback)
{
    this->iteration_finished_callback = iteration_finished_callback;
}

void TGVWidget::on_perform_button_clicked()
{
    this->processInWorkerThread();
}

ITKImage TGVWidget::processImage(ITKImage image)
{
    const float alpha0 = this->ui->alpha0_spinbox->value();
    const float alpha1 = this->ui->alpha1_spinbox->value();
    const float lambda = this->ui->lambda_spinbox->value();
    const uint iteration_count = this->ui->iteration_count_spinbox->value();
    const uint paint_iteration_interval = this->ui->paint_iteration_interval_spinbox->value();

    bool perform_on_gpu = this->ui->gpu_radio_button->isChecked();

    if(perform_on_gpu) {
        bool perform_using_thrust = this->ui->thrust_runtime_checkbox->isChecked();

        if(perform_using_thrust)
            return TGVProcessor::processTVL2GPUThrust(image, lambda, iteration_count,
                                                paint_iteration_interval,
                                                this->iteration_finished_callback);

        if(this->ui->tgv1_l2_algorithm_checkbox->isChecked())
            return TGVProcessor::processTVL2GPUCuda(image, lambda,
                                                    alpha0,
                                                    alpha1,
                                                    iteration_count,
                                                paint_iteration_interval,
                                                this->iteration_finished_callback);
        if(this->ui->tgv1_l1_algorithm_checkbox->isChecked())
            return TGVProcessor::processTVL1GPUCuda(image, lambda,
                                                    alpha0,
                                                    alpha1,
                                                    iteration_count,
                                                paint_iteration_interval,
                                                this->iteration_finished_callback);
    }

    return TGVProcessor::processTVL2CPU(image,
                                        lambda, iteration_count,
                                        paint_iteration_interval,
                                        this->iteration_finished_callback);
}
