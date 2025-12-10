#include "LivestreamCanvas.h"
#include <QSettings>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QVBoxLayout>
#include <QPainter>

// OverlayWidget paint event implementation
void OverlayWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    
    // Clear with transparent background first
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    // Draw overlay if canvas exists
    if (canvas) {
        painter.setRenderHint(QPainter::Antialiasing);
        canvas->drawOverlay(painter);
    }
}

LivestreamCanvas::LivestreamCanvas(QWidget *parent)
    : CanvasWidget(parent)
    , webView(nullptr)
    , overlayWidget(nullptr)
{
    // Create web view for VDO.Ninja (initially hidden)
    webView = new QWebEngineView(this);
    webView->setGeometry(rect());
    webView->hide();
    
    // Enable autoplay for video
    webView->settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    webView->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    
    // Create transparent overlay widget on top of webView
    overlayWidget = new OverlayWidget(this);
    overlayWidget->setCanvas(this);
    overlayWidget->setGeometry(rect());
    overlayWidget->raise();  // Always on top
    overlayWidget->hide();
    
    loadSettings();
}

LivestreamCanvas::~LivestreamCanvas()
{
    saveSettings();
}

void LivestreamCanvas::showBibleVerse(const QString &reference, const QString &text)
{
    formatBibleText(reference, text);
}

void LivestreamCanvas::showLyrics(const QString &songTitle, const QString &lyrics)
{
    formatLyricsText(songTitle, lyrics);
}
void LivestreamCanvas::setVdoNinjaUrl(const QString &url)
{
    vdoNinjaUrl = url;
    if (!url.isEmpty()) {
        webView->setUrl(QUrl(url));
        webView->show();
        overlayWidget->show();
        overlayWidget->raise();  // Ensure overlay is on top
        update();
    } else {
        clearVdoNinja();
    }
}

void LivestreamCanvas::clearVdoNinja()
{
    vdoNinjaUrl.clear();
    webView->hide();
    update();
}

void LivestreamCanvas::formatBibleText(const QString &reference, const QString &text)
{
    showOverlayWithReference(reference, text);
    
    // Update overlay widget if VDO.Ninja is active
    if (hasVdoNinja() && overlayWidget) {
        overlayWidget->update();
    }
}

void LivestreamCanvas::formatLyricsText(const QString &songTitle, const QString &lyrics)
{
    // For livestream, might want to show song title
    showOverlay(lyrics);
    
    // Update overlay widget if VDO.Ninja is active
    if (hasVdoNinja() && overlayWidget) {
        overlayWidget->update();
    }
}

void LivestreamCanvas::loadSettings()
{
    QSettings settings("SimplePresenter", "SimplePresenter");
    settings.beginGroup("LivestreamCanvas");
    
    OverlayConfig config = getOverlayConfig();
    
    // Load overlay position
    config.geometry = settings.value("overlayGeometry", config.geometry).toRect();
    
    // Load font settings (might be different from projection)
    QFont defaultFont("Arial", 48);
    defaultFont.setBold(true);
    config.font = settings.value("font", defaultFont).value<QFont>();
    
    QFont defaultRefFont("Arial", 36);
    defaultRefFont.setBold(true);
    config.referenceFont = settings.value("referenceFont", defaultRefFont).value<QFont>();
    
    // Load colors
    config.textColor = settings.value("textColor", config.textColor).value<QColor>();
    config.referenceColor = settings.value("referenceColor", config.referenceColor).value<QColor>();
    QColor storedBg = settings.value("backgroundColor", config.backgroundColor).value<QColor>();
    int storedOpacity = settings.value("backgroundOpacity", -1).toInt();
    if (storedOpacity >= 0) {
        int clampedOpacity = qBound(0, storedOpacity, 100);
        storedBg.setAlpha(qRound(clampedOpacity * 255.0 / 100.0));
    } else {
        bool transparentBg = settings.value("backgroundTransparent", false).toBool();
        if (transparentBg) {
            storedBg = QColor(0, 0, 0, 0);
        } else if (storedBg.alpha() == 0) {
            storedBg.setAlpha(180);
        }
    }
    config.backgroundColor = storedBg;
    config.textHighlightEnabled = settings.value("textHighlightEnabled", config.textHighlightEnabled).toBool();
    config.textHighlightColor = settings.value("textHighlightColor", config.textHighlightColor).value<QColor>();
    config.referenceHighlightEnabled = settings.value("referenceHighlightEnabled", config.referenceHighlightEnabled).toBool();
    config.referenceHighlightColor = settings.value("referenceHighlightColor", config.referenceHighlightColor).value<QColor>();
    config.textHighlightCornerRadius = qBound(0, settings.value("textHighlightCornerRadius", config.textHighlightCornerRadius).toInt(), 200);
    config.referenceHighlightCornerRadius = qBound(0, settings.value("referenceHighlightCornerRadius", config.referenceHighlightCornerRadius).toInt(), 200);
    
    // Load alignment
    config.alignment = Qt::Alignment(settings.value("alignment", int(config.alignment)).toInt());
    config.referenceAlignment = Qt::Alignment(settings.value("referenceAlignment", int(config.referenceAlignment)).toInt());
    config.padding = settings.value("padding", config.padding).toInt();
    config.opacity = settings.value("opacity", config.opacity).toFloat();
    
    // Load vertical position
    int vPos = settings.value("verticalPosition", 1).toInt();
    config.verticalPosition = static_cast<VerticalPosition>(vPos);
    
    // Load reference position
    int refPos = settings.value("referencePosition", 0).toInt();
    config.referencePosition = static_cast<ReferencePosition>(refPos);
    
    // Load VDO.Ninja URL
    QString url = settings.value("vdoNinjaUrl").toString();
    if (!url.isEmpty()) {
        setVdoNinjaUrl(url);
    }
    
    // Load background (if not using VDO.Ninja)
    if (url.isEmpty()) {
        QString bgType = settings.value("backgroundType", "color").toString();
        if (bgType == "color") {
            QColor color = settings.value("backgroundColor", QColor(Qt::black)).value<QColor>();
            setBackgroundColor(color);
        } else if (bgType == "image") {
            QString imagePath = settings.value("backgroundImage").toString();
            if (!imagePath.isEmpty()) {
                setBackgroundImage(imagePath);
            }
        } else if (bgType == "video") {
            QString videoPath = settings.value("backgroundVideo").toString();
            bool loop = settings.value("backgroundVideoLoop", true).toBool();
            if (!videoPath.isEmpty()) {
                setBackgroundVideo(videoPath, loop);
            }
        }
    }
    
    setOverlayConfig(config);
    
    settings.endGroup();
}

void LivestreamCanvas::saveSettings()
{
    QSettings settings("SimplePresenter", "SimplePresenter");
    settings.beginGroup("LivestreamCanvas");
    
    OverlayConfig config = getOverlayConfig();
    
    settings.setValue("overlayGeometry", config.geometry);
    settings.setValue("font", config.font);
    settings.setValue("textColor", config.textColor);
    settings.setValue("referenceColor", config.referenceColor);
    QColor bgColor = config.backgroundColor;
    int opacityPercent = qRound(bgColor.alpha() * 100.0 / 255.0);
    settings.setValue("backgroundColor", bgColor);
    settings.setValue("backgroundOpacity", opacityPercent);
    settings.setValue("backgroundTransparent", opacityPercent == 0);
    settings.setValue("backgroundCornerRadius", config.textHighlightCornerRadius);
    settings.setValue("textHighlightEnabled", config.textHighlightEnabled);
    settings.setValue("textHighlightColor", config.textHighlightColor);
    settings.setValue("textHighlightCornerRadius", config.textHighlightCornerRadius);
    settings.setValue("referenceHighlightEnabled", config.referenceHighlightEnabled);
    settings.setValue("referenceHighlightColor", config.referenceHighlightColor);
    settings.setValue("referenceHighlightCornerRadius", config.referenceHighlightCornerRadius);
    settings.setValue("alignment", int(config.alignment));
    settings.setValue("referenceAlignment", int(config.referenceAlignment));
    settings.setValue("padding", config.padding);
    settings.setValue("verticalPosition", int(config.verticalPosition));
    settings.setValue("referencePosition", int(config.referencePosition));
    settings.setValue("vdoNinjaUrl", vdoNinjaUrl);
    
    settings.endGroup();
}

void LivestreamCanvas::resizeEvent(QResizeEvent *event)
{
    CanvasWidget::resizeEvent(event);
    
    // Keep webView and overlay sized to fill the canvas
    if (webView) {
        webView->setGeometry(rect());
    }
    if (overlayWidget) {
        overlayWidget->setGeometry(rect());
        overlayWidget->raise();  // Keep on top
    }
}

void LivestreamCanvas::paintEvent(QPaintEvent *event)
{
    // Always use normal canvas painting
    // The webView is a separate widget that renders independently
    CanvasWidget::paintEvent(event);
    
    // If VDO.Ninja is active, also paint overlay on top
    if (hasVdoNinja() && webView->isVisible() && overlayWidget && overlayWidget->isVisible()) {
        // Trigger overlay widget to repaint
        overlayWidget->update();
    }
}
