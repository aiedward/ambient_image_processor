#ifndef BASEMODULEWIDGET_H
#define BASEMODULEWIDGET_H

#include <functional>
#include <thread>

#include "ITKImage.h"
#include "ImageWidget.h"

#include <QWidget>

class BaseModuleWidget : public QWidget
{
    Q_OBJECT
public:
    BaseModuleWidget(QString title, QWidget *parent);

    typedef std::function<void(ITKImage)> ResultProcessor;
    typedef std::function<ITKImage()> SourceImageFetcher;
private:
    SourceImageFetcher source_image_fetcher;
    ResultProcessor result_processor;

    std::thread* worker_thread;

    QString title;
signals:
    void fireWorkerFinished();
private slots:
    void handleWorkerFinished();

protected:
    virtual ITKImage processImage(ITKImage image);
    void processInWorkerThread();

    ITKImage getSourceImage() const;

private:
    std::function<void(QString)> status_text_processor;
protected:
    void setStatusText(QString text);
public:
    virtual void registerModule(ImageWidget* image_widget);

    QString getTitle() const;
    virtual void connectTo(BaseModuleWidget* other);
};

#endif // BASEMODULEWIDGET_H
