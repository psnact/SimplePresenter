#ifndef LIVESTREAMCANVAS_H
#define LIVESTREAMCANVAS_H

#include "CanvasWidget.h"
#include <QWidget>
#include <QPainter>

class QWebEngineView;

// Transparent overlay widget that paints on top of webView
class OverlayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
    }
    
    void setCanvas(class LivestreamCanvas *c) { canvas = c; }
    
protected:
    void paintEvent(QPaintEvent *event) override;
    
private:
    class LivestreamCanvas *canvas;
};

class LivestreamCanvas : public CanvasWidget
{
    Q_OBJECT

public:
    explicit LivestreamCanvas(QWidget *parent = nullptr);
    ~LivestreamCanvas();
    
    // Content display methods
    void showBibleVerse(const QString &reference, const QString &text);
    void showLyrics(const QString &songTitle, const QString &lyrics);
    
    // VDO.Ninja integration
    void setVdoNinjaUrl(const QString &url);
    void clearVdoNinja();
    bool hasVdoNinja() const { return !vdoNinjaUrl.isEmpty(); }
    
    // Load livestream-specific settings
    void loadSettings();
    void saveSettings();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    friend class OverlayWidget;  // Allow OverlayWidget to access drawOverlay

private:
    void formatBibleText(const QString &reference, const QString &text);
    void formatLyricsText(const QString &songTitle, const QString &lyrics);
    
    QString vdoNinjaUrl;
    QWebEngineView *webView;
    OverlayWidget *overlayWidget;  // Transparent widget for drawing overlay on top
};

#endif // LIVESTREAMCANVAS_H
