#ifndef BACKGROUNDRENDERER_H
#define BACKGROUNDRENDERER_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QColor>

class BackgroundRenderer : public QObject
{
    Q_OBJECT

public:
    explicit BackgroundRenderer(QObject *parent = nullptr);
    ~BackgroundRenderer();
    
    // Render background to image
    QImage render(int width, int height);
    
    // Background setters
    void setSolidColor(const QColor &color);
    void setImage(const QString &imagePath);
    void setVideo(const QString &videoPath);
    void clear();

private:
    enum Type { None, SolidColor, Image, Video };
    Type type;
    QColor solidColor;
    QString imagePath;
    QString videoPath;
};

#endif // BACKGROUNDRENDERER_H
