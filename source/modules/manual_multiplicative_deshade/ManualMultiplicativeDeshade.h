#ifndef MANUALMULTIPLICATIVEDESHADE_H
#define MANUALMULTIPLICATIVEDESHADE_H

#include <QWidget>

#include "BaseModuleWidget.h"

namespace Ui {
class ManualMultiplicativeDeshade;
}

class ManualMultiplicativeDeshade : public BaseModuleWidget
{
    Q_OBJECT

public:
    ManualMultiplicativeDeshade(QString title, QWidget *parent);
    ~ManualMultiplicativeDeshade();

    void registerModule(ImageWidget *image_widget);
private slots:
    void mousePressedOnImage(Qt::MouseButton button, ITKImage::Index position);

    void on_kernel_sigma_spinbox_editingFinished();
    void on_kernel_size_spinbox_editingFinished();
    void on_kernel_maximum_spinbox_editingFinished();

    void on_reset_shading_button_clicked();

private:
    Ui::ManualMultiplicativeDeshade *ui;

    ITKImage shading;
    ITKImage kernel;

    ITKImage::Index cursor_position;
    bool increase;

    void initShading();
    void generateKernel();

protected:
    ITKImage processImage(ITKImage image);
};

#endif // MANUALMULTIPLICATIVEDESHADE_H
