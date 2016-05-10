#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "ITKToQImageConverter.h"
#include "ITKImageProcessor.h"
#include <QDateTime>

#include <QPainter>

MainWindow::MainWindow(std::string image_path) :
    QMainWindow(NULL),
    ui(new Ui::MainWindow),
    source_image_path(image_path)
{
    ui->setupUi(this);

    if(QFile(QString::fromStdString(image_path)).exists())
    {
        ImageType::Pointer input_image = ITKImageProcessor::read(image_path);
        this->ui->image_widget->setImage(input_image);
        this->ui->image_widget->showSliceControl();

     //   this->ui->output_widget->showImageOnly();
        this->ui->image_widget->setOutputWidget(this->ui->output_widget);
        this->ui->output_widget->connectSliceControlTo(this->ui->image_widget);
        this->ui->output_widget->connectProfileLinesTo(this->ui->image_widget);
        this->ui->output_widget->setPage(0);

        this->ui->image_widget->setOutputWidget2(this->ui->output_widget2);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
