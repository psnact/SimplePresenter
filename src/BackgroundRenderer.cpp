#include "BackgroundRenderer.h"
#include <QPainter>

BackgroundRenderer::BackgroundRenderer(QObject *parent)
    : QObject(parent)
    , type(SolidColor)
    , solidColor(Qt::black)
{
}

BackgroundRenderer::~BackgroundRenderer()
{
}

QImage BackgroundRenderer::render(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    QPainter painter(&image);
    
    switch (type) {
        case SolidColor:
            painter.fillRect(image.rect(), solidColor);
            break;
            
        case Image:
            if (!imagePath.isEmpty()) {
                QImage bgImage(imagePath);
                if (!bgImage.isNull()) {
                    QImage scaled = bgImage.scaled(width, height, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                    int x = (width - scaled.width()) / 2;
                    int y = (height - scaled.height()) / 2;
                    painter.drawImage(x, y, scaled);
                } else {
                    painter.fillRect(image.rect(), Qt::black);
                }
            } else {
                painter.fillRect(image.rect(), Qt::black);
            }
            break;
            
        case Video:
            // Video rendering handled by CanvasWidget
            painter.fillRect(image.rect(), Qt::black);
            break;
            
        case None:
        default:
            painter.fillRect(image.rect(), Qt::black);
            break;
    }
    
    return image;
}

void BackgroundRenderer::setSolidColor(const QColor &color)
{
    type = SolidColor;
    solidColor = color;
}

void BackgroundRenderer::setImage(const QString &path)
{
    type = Image;
    imagePath = path;
}

void BackgroundRenderer::setVideo(const QString &path)
{
    type = Video;
    videoPath = path;
}

void BackgroundRenderer::clear()
{
    type = None;
}
