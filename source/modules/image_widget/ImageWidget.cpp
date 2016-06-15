#include "ImageWidget.h"
#include "ui_ImageWidget.h"

#include <QPainter>
#include <QFileDialog>
#include <QDateTime>
#include <QMenuBar>

#include "ImageViewWidget.h"
#include "SliceControlWidget.h"
#include "LineProfileWidget.h"
#include "UnsharpMaskingWidget.h"
#include "MultiScaleRetinexWidget.h"
#include "NonLocalGradientWidget.h"
#include "RegionGrowingSegmentationWidget.h"
#include "HistogramWidget.h"
#include "ImageInformationWidget.h"
#include "ShrinkWidget.h"
#include "SplineInterpolationWidget.h"
#include "ThresholdFilterWidget.h"
#include "ExtractWidget.h"
#include "BilateralFilterWidget.h"
#include "DeshadeSegmentedWidget.h"
#include "TGVWidget.h"
#include "TGVL1ThresholdGradientWidget.h"
#include "ManualMultiplicativeDeshade.h"
#include "TGVLambdasWidget.h"
#include "BinaryOperationsWidget.h"
#include "ConvolutionWidget.h"
#include "RegionCurvatureEdgeCorrection.h"
#include "RescaleIntensityWidget.h"
#include "TGVShadingGrowingWidget.h"
#include "TGVDeshadeWidget.h"
#include "UnaryOperationsWidget.h"
#include "MorphologicalFilterWidget.h"
#include "ImageViewControlWidget.h"
#include "TGVNonParametricDeshadeWidget.h"
#include "TGVDeshadeMetricPlotWidget.h"
#include "TGVDeshadeIntegralMetricPlotWidget.h"

ImageWidget::ImageWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageWidget),
    image(nullptr),
    output_widget(this),
    output_widget2(this)
{
    ui->setupUi(this);

    connect(this, SIGNAL(fireStatusTextChange(QString)),
            this, SLOT(handleStatusTextChange(QString)));

    qRegisterMetaType<ITKImage>("ITKImage");
    qRegisterMetaType<ITKImage>("ITKImage::Index");

    connect(this, &ImageWidget::fireImageChange,
            this, &ImageWidget::handleImageChange);


    this->ui->operations_panel->setVisible(false);

    // define modules
    auto module_parent = this->ui->operations_panel;

    auto region_growing_segmentation_widget =
            new RegionGrowingSegmentationWidget("Region Growing Segmentation", module_parent);
    auto non_local_gradient_widget = new NonLocalGradientWidget("Non-local Gradient", module_parent);
    auto deshade_segmented_widget = new DeshadeSegmentedWidget("Deshade Segmented", module_parent);
    auto tgv_widget = new TGVWidget("TGV Filter", module_parent);
    auto tgv_lambdas_widget = new TGVLambdasWidget("TGV Lambdas", module_parent);
    auto tgv_shading_growing_widget = new TGVShadingGrowingWidget("TGV Shading Growing", module_parent);
    auto tgv_deshade_widget = new TGVDeshadeWidget("TGV Deshade", module_parent);
    auto tgv_non_parametric_deshade_widget = new TGVNonParametricDeshadeWidget("TGV Automatic Deshade", module_parent);
    auto tgv_deshade_metric_plot_widget = new TGVDeshadeMetricPlotWidget("TGV Deshade Metric Plot", module_parent);
    auto tgv_deshade_integral_metric_plot_widget = new TGVDeshadeIntegralMetricPlotWidget("TGV Deshade Integral Metric Plot", module_parent);

    this->image_view_widget = new ImageViewWidget("Image View", this->ui->image_frame);
    this->slice_control_widget = new SliceControlWidget("Slice Control", this->ui->slice_control_widget_frame);

    auto image_view_control_widget = new ImageViewControlWidget("Image View Control", module_parent);

    modules.push_back(this->image_view_widget);
    modules.push_back(this->slice_control_widget);
    modules.push_back(new ImageInformationWidget("Image Information", module_parent));
    modules.push_back(image_view_control_widget);
    modules.push_back(new LineProfileWidget("Line Profile", module_parent));
    modules.push_back(new HistogramWidget("Histogram", module_parent));
    modules.push_back(new BinaryOperationsWidget("Binary Operations", module_parent));
    modules.push_back(new UnaryOperationsWidget("Unary Operations", module_parent));
    modules.push_back(new ThresholdFilterWidget("Threshold", module_parent));
    modules.push_back(new MorphologicalFilterWidget("Morphological Filter", module_parent));
    modules.push_back(new ConvolutionWidget("3x3x3 Convolution", module_parent));
    modules.push_back(new RescaleIntensityWidget("Rescale Intensity", module_parent));
    modules.push_back(new ShrinkWidget("Shrink", module_parent));
    modules.push_back(new ExtractWidget("Extract", module_parent));
    modules.push_back(non_local_gradient_widget);
    modules.push_back(region_growing_segmentation_widget);
    modules.push_back(new SplineInterpolationWidget("Spline Interpolation", module_parent));
    modules.push_back(new BilateralFilterWidget("Bilateral Filter", module_parent));
    modules.push_back(tgv_widget);
    modules.push_back(tgv_lambdas_widget);

    modules.push_back(new UnsharpMaskingWidget("Unsharp Masking", module_parent));
    modules.push_back(new MultiScaleRetinexWidget("Multiscale Retinex", module_parent));

    modules.push_back(new RegionCurvatureEdgeCorrection("Region Curvature Edge Correction", module_parent));
    modules.push_back(new ManualMultiplicativeDeshade("Manual Multiplicative Deshade", module_parent));
    modules.push_back(new TGVL1ThresholdGradientWidget("TGVL1 Thresholded Gradient", module_parent));
    modules.push_back(deshade_segmented_widget);
    modules.push_back(tgv_shading_growing_widget);
    modules.push_back(tgv_deshade_widget);
    modules.push_back(tgv_deshade_metric_plot_widget);
    modules.push_back(tgv_deshade_integral_metric_plot_widget);
    modules.push_back(tgv_non_parametric_deshade_widget);

    // register modules and add widget modules
    module_parent->hide();
    module_parent->setUpdatesEnabled(false);
    uint index = 0;
    for(auto module : modules)
    {
        auto widget = dynamic_cast<BaseModuleWidget*>(module);
        if(widget != nullptr &&
                widget != slice_control_widget &&
                widget != image_view_widget)
            module_parent->insertTab(index++, widget, module->getTitle());

        std::cout << "registering module: " << module->getTitle().toStdString() << std::endl;
        module->registerModule(this);
    }

    this->ui->image_frame->layout()->addWidget(image_view_widget);
    this->ui->slice_control_widget_frame->layout()->addWidget(slice_control_widget);

    module_parent->setUpdatesEnabled(true);
    module_parent->show();

    // create menu entry for widget modules
    QMenuBar* menu_bar = new QMenuBar();
    this->image_menu = new QMenu("Image");
    QAction* load_action = image_menu->addAction("Load File");
    this->connect(load_action, &QAction::triggered, this, [this]() {
        this->on_load_button_clicked();
    });
    QAction* save_action = image_menu->addAction("Save File");
    this->connect(save_action, &QAction::triggered, this, [this]() {
        this->on_save_button_clicked();
    });
    image_menu->addSeparator();
    QAction* save_with_overlays_action = image_menu->addAction("Save File with Overlays");
    this->connect(save_with_overlays_action, &QAction::triggered, this, [this]() {
        this->image_view_widget->save_file_with_overlays();
    });

    auto line_profile_module = this->getModuleByName("Line Profile");
    if(line_profile_module)
    {
        QAction* save_line_profile_action = image_menu->addAction("Save Line Profile");
        this->connect(save_line_profile_action, &QAction::triggered, this, [line_profile_module]() {
            auto line_profile_module_casted = dynamic_cast<LineProfileWidget*>(line_profile_module);
            line_profile_module_casted->save_to_file();
        });
    }
    image_menu->addSeparator();
    QAction* load_hsv = image_menu->addAction("Load Color File HSV");
    this->connect(load_hsv, &QAction::triggered, this, [this]() {
        this->load_hsv_clicked();
    });
    QAction* save_hsv = image_menu->addAction("Save into Color File HSV");
    this->connect(save_hsv, &QAction::triggered, this, [this]() {
        this->save_hsv_clicked();
    });
    image_menu->addSeparator();
    QAction* load_color_to_view_only = image_menu->addAction("Load Color File to View only");
    this->connect(load_color_to_view_only, &QAction::triggered, this, [this]() {
        this->image_view_widget->load_color_to_view_only_clicked();
    });

    menu_bar->addMenu(image_menu);
    QMenu *tools_menu = new QMenu("Tools");
    for(auto module : modules)
    {
        auto widget = dynamic_cast<BaseModuleWidget*>(module);

        if(widget == nullptr ||
           widget == slice_control_widget ||
           widget == image_view_widget)
            continue;

        QAction* module_action = tools_menu->addAction(module->getTitle());

        if(widget->getTitle() == "Histogram" ||
           widget->getTitle() == "Rescale Intensity" ||
           widget->getTitle() == "Extract" ||
           widget->getTitle() == "TGV Lambdas" ||
           widget->getTitle() == "Multiscale Retinex" )
            tools_menu->addSeparator();

        this->connect(module_action, &QAction::triggered, this, [this, widget]() {
            this->ui->operations_panel->setCurrentWidget(widget);
        });
    }
    menu_bar->addMenu(tools_menu);
    this->layout()->setMenuBar(menu_bar);

    // connect modules...

    connect(image_view_control_widget, &ImageViewControlWidget::doRescaleChanged,
            this->image_view_widget, &ImageViewWidget::doRescaleChanged);
    connect(image_view_control_widget, &ImageViewControlWidget::doMultiplyChanged,
            this->image_view_widget, &ImageViewWidget::doMultiplyChanged);

    deshade_segmented_widget->setSegmentsFetcher([region_growing_segmentation_widget]() {
        return region_growing_segmentation_widget->getSegments();
    });
    deshade_segmented_widget->setLabelImageFetcher([region_growing_segmentation_widget]() {
        return region_growing_segmentation_widget->getLabelImage();
    });

    // iteration finished callback...
    auto iteration_finished_callback = [this](uint index, uint count, ITKImage u) {
        emit this->fireStatusTextChange(QString("iteration %1 / %2").arg(
                                            QString::number(index+1),
                                            QString::number(count)));
        emit this->output_widget->fireImageChange(u.getPointer());
        return false;
    };
    tgv_widget->setIterationFinishedCallback(iteration_finished_callback);
    tgv_lambdas_widget->setIterationFinishedCallback(iteration_finished_callback);
    tgv_shading_growing_widget->setIterationFinishedCallback(iteration_finished_callback);
    tgv_deshade_widget->setIterationFinishedCallback(iteration_finished_callback);
    tgv_deshade_metric_plot_widget->setIterationFinishedCallback(iteration_finished_callback);
    tgv_deshade_integral_metric_plot_widget->setIterationFinishedCallback(iteration_finished_callback);
}

ImageWidget::~ImageWidget()
{
    delete ui;
}


void ImageWidget::setImage(ITKImage image)
{
    this->image = image.clone();

    this->setMinimumSizeToImage();

    emit this->imageChanged(this->image);
}

BaseModule* ImageWidget::getModuleByName(QString module_title) const
{
    for(auto module : modules)
        if(module->getTitle() == module_title)
            return module;
    return nullptr;

}
void ImageWidget::connectModule(QString module_title, ImageWidget* other_image_widget)
{
    auto module1 = this->getModuleByName(module_title);
    auto module2 = other_image_widget->getModuleByName(module_title);

    if(module1 == nullptr || module2 == nullptr)
        return;

    module1->connectTo(module2);
}

void ImageWidget::on_load_button_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, "open volume file");
    if(file_name == QString::null || !QFile(file_name).exists())
        return;

    this->setImage(ITKImage::read(file_name.toStdString()));
}

void ImageWidget::on_save_button_clicked()
{
    if(this->image.isNull())
        return;

    QString file_name = QFileDialog::getSaveFileName(this, "save volume file");
    if(file_name.isNull())
        return;

    this->image.write(file_name.toStdString());
}

void ImageWidget::save_hsv_clicked()
{
    if(this->image.isNull())
        return;

    QString template_file_name = QFileDialog::getOpenFileName(this, "open template color file for the H and S channel");
    if(template_file_name == QString::null || !QFile(template_file_name).exists())
        return;

    QString file_name = QFileDialog::getSaveFileName(this, "save V channel into color file");
    if(file_name.isNull())
        return;

    QFile::copy(template_file_name, file_name);

    this->image.write_hsv(file_name.toStdString());
}

void ImageWidget::load_hsv_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, "open volume file");
    if(file_name == QString::null || !QFile(file_name).exists())
        return;


    this->setImage(ITKImage::read_hsv(file_name.toStdString()));
}

void ImageWidget::setMinimumSizeToImage()
{
    if(this->image.isNull())
        return;

    const int border = 50;
    this->ui->image_frame->setMinimumSize(
                this->image.width + border*2,
                this->image.height + border*2);
}

void ImageWidget::setOutputWidget(ImageWidget* output_widget)
{
    this->output_widget = output_widget;

    output_widget->image_menu->addSeparator();
    auto action = output_widget->image_menu->addAction("Swap Input and Output");
    this->connect(action, &QAction::triggered, this, [this, output_widget]() {
        auto source_image = this->getImage();
        this->setImage(this->output_widget->getImage());
        output_widget->setImage(source_image);
    });
    this->image_menu->addSeparator();
    this->image_menu->addAction(action);
}

ImageWidget* ImageWidget::getOutputWidget() const
{
    return this->output_widget;
}

void ImageWidget::setOutputWidget2(ImageWidget* output_widget)
{
    this->output_widget2 = output_widget;
}

void ImageWidget::setOutputWidget3(ImageWidget* output_widget)
{
    this->output_widget3 = output_widget;
}


void ImageWidget::handleStatusTextChange(QString text)
{
    this->ui->status_bar->setText(text);
    this->ui->status_bar->repaint();
}

void ImageWidget::handleImageChange(ITKImage image)
{
    this->setImage(image);
}

void ImageWidget::setPage(unsigned char page_index)
{
    this->ui->operations_panel->setCurrentIndex(page_index);
}
