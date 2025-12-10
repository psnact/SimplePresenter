#include "CanvasWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QVideoFrame>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextBlock>
#include <QTextLayout>
#include <QAbstractTextDocumentLayout>
#include <QPainterPath>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

QString alignmentToCss(Qt::Alignment alignment)
{
    if (alignment.testFlag(Qt::AlignLeft)) {
        return "left";
    }
    if (alignment.testFlag(Qt::AlignRight)) {
        return "right";
    }
    return "center";
}

QString colorToCss(const QColor &color)
{
    return QString("rgba(%1,%2,%3,%4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(QString::number(color.alphaF(), 'f', 2));
}

static void fillRoundedRect(QPainter &painter, const QRectF &rect, const QColor &color, int radius)
{
    if (color.alpha() == 0 || rect.isEmpty()) {
        return;
    }

    painter.save();
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);

    qreal clampedRadius = std::clamp<qreal>(radius, 0.0, std::min(rect.width(), rect.height()) / 2.0);
    if (clampedRadius > 0.0) {
        QPainterPath path;
        path.addRoundedRect(rect, clampedRadius, clampedRadius);
        painter.drawPath(path);
    } else {
        painter.drawRect(rect);
    }

    painter.restore();
}

static void drawDocumentHighlightBackground(QPainter &painter,
                                            QTextDocument &doc,
                                            const OverlayConfig &config,
                                            const QColor &fillColor,
                                            bool drawBorder = false,
                                            int borderThickness = 0)
{
    const bool hasBorder = drawBorder && borderThickness > 0;
    const bool hasFill = fillColor.alpha() > 0;
    if (!hasFill && !hasBorder) {
        return;
    }

    if (config.textLineSpacingFactor <= 1.01) {
        QRectF docRect(QPointF(0, 0), doc.size());
        QRectF backgroundRect = docRect.adjusted(-config.padding / 2.0,
                                                 -config.padding / 2.0,
                                                 config.padding / 2.0,
                                                 config.padding / 2.0);
        if (hasFill) {
            fillRoundedRect(painter, backgroundRect, fillColor, config.textHighlightCornerRadius);
        }
        if (hasBorder) {
            qreal docWidth = doc.size().width();
            qreal longestLine = 0.0;
            qreal top = std::numeric_limits<qreal>::max();
            qreal bottom = std::numeric_limits<qreal>::lowest();
            bool foundLine = false;

            if (QAbstractTextDocumentLayout *layout = doc.documentLayout()) {
                for (QTextBlock block = doc.begin(); block.isValid(); block = block.next()) {
                    if (QTextLayout *blockLayout = block.layout()) {
                        QRectF blockRect = layout->blockBoundingRect(block);
                        for (int i = 0; i < blockLayout->lineCount(); ++i) {
                            QTextLine line = blockLayout->lineAt(i);
                            if (!line.isValid()) {
                                continue;
                            }

                            longestLine = std::max(longestLine, line.naturalTextWidth());
                            const qreal lineTop = blockRect.top() + line.position().y();
                            const qreal lineBottom = lineTop + line.height();
                            top = std::min(top, lineTop);
                            bottom = std::max(bottom, lineBottom);
                            foundLine = true;
                        }
                    }
                }
            }

            if (!foundLine) {
                longestLine = std::max(doc.idealWidth(), docWidth);
                top = 0.0;
                bottom = doc.size().height();
            }

            qreal halfContentWidth = longestLine / 2.0;
            if (halfContentWidth <= 0.0) {
                halfContentWidth = docWidth / 2.0;
            }

            qreal centerX;
            Qt::Alignment align = config.alignment;
            if (align.testFlag(Qt::AlignRight)) {
                centerX = docWidth - halfContentWidth;
            } else if (align.testFlag(Qt::AlignHCenter) || align.testFlag(Qt::AlignCenter)) {
                centerX = docWidth / 2.0;
            } else {
                centerX = halfContentWidth;
            }

            qreal halfWidth = halfContentWidth + config.textBorderPaddingHorizontal;
            qreal topEdge = top - config.textBorderPaddingVertical;
            qreal bottomEdge = bottom + config.textBorderPaddingVertical;
            if (bottomEdge <= topEdge) {
                bottomEdge = topEdge + longestLine * 0.1 + config.textBorderPaddingVertical * 2.0;
            }

            QRectF borderRect(centerX - halfWidth,
                              topEdge,
                              halfWidth * 2.0,
                              bottomEdge - topEdge);
            QPen pen(config.textBorderColor);
            pen.setWidth(borderThickness);
            pen.setJoinStyle(Qt::MiterJoin);
            pen.setCapStyle(Qt::SquareCap);
            painter.save();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(pen);
            painter.drawRect(borderRect);
            painter.restore();
        }
        return;
    }

    if (QAbstractTextDocumentLayout *layout = doc.documentLayout()) {
        for (QTextBlock block = doc.begin(); block.isValid(); block = block.next()) {
            QRectF blockRect = layout->blockBoundingRect(block);
            if (blockRect.isEmpty()) {
                continue;
            }
            QRectF backgroundRect = blockRect.adjusted(-config.padding / 2.0,
                                                       -config.padding / 2.0,
                                                       config.padding / 2.0,
                                                       config.padding / 2.0);
            if (hasFill) {
                fillRoundedRect(painter, backgroundRect, fillColor, config.textHighlightCornerRadius);
            }
        }
    }
}

} // namespace

CanvasWidget::CanvasWidget(QWidget *parent)
    : QWidget(parent)
    , backgroundType(BackgroundType::SolidColor)
    , backgroundColor(Qt::black)
    , backgroundImagePreserveSize(false)
    , videoPlayer(nullptr)
    , videoSink(nullptr)
    , audioOutputDevice(nullptr)
    , notesHtml()
    , notesVisibleFlag(false)
    , needsRedraw(true)
    , fadeTimer(nullptr)
    , fadeActive(false)
    , fadeProgress(0.0)
    , fadeDurationMs(250)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Initialize video player
    videoPlayer = new QMediaPlayer(this);
    audioOutputDevice = new QAudioOutput(this);
    videoPlayer->setAudioOutput(audioOutputDevice);
    videoSink = new QVideoSink(this);
    videoPlayer->setVideoSink(videoSink);

    connect(videoSink, &QVideoSink::videoFrameChanged,
            this, &CanvasWidget::onVideoFrameChanged);

    fadeTimer = new QTimer(this);
    fadeTimer->setInterval(16);
    connect(fadeTimer, &QTimer::timeout, this, [this]() {
        if (!fadeActive) {
            fadeTimer->stop();
            return;
        }
        if (fadeDurationMs <= 0) {
            fadeActive = false;
            fadeTimer->stop();
            needsRedraw = true;
            update();
            return;
        }
        fadeProgress += 16.0 / static_cast<double>(fadeDurationMs);
        if (fadeProgress >= 1.0) {
            fadeProgress = 1.0;
            fadeActive = false;
            fadeTimer->stop();
        }
        needsRedraw = true;
        update();
    });
}

CanvasWidget::~CanvasWidget()
{
    if (videoPlayer) {
        videoPlayer->stop();
    }
}

void CanvasWidget::setBackgroundColor(const QColor &color)
{
    if (videoPlayer) {
        videoPlayer->stop();
        videoPlayer->setSource(QUrl());
    }
    backgroundColor = color;
    backgroundType = BackgroundType::SolidColor;
    needsRedraw = true;
    update();
}

void CanvasWidget::setBackgroundImageWithFade(const QString &imagePath, int durationMs)
{
    if (durationMs <= 0 || !isVisible()) {
        setBackgroundImage(imagePath);
        return;
    }

    fadePrevFrame = grab();
    fadeDurationMs = durationMs;
    fadeProgress = 0.0;
    fadeActive = true;

    setBackgroundImage(imagePath);

    fadeTimer->start();
}

void CanvasWidget::setBackgroundImage(const QString &imagePath, bool preserveOriginalSize)
{
    if (videoPlayer) {
        videoPlayer->stop();
        videoPlayer->setSource(QUrl());
    }
    QPixmap pixmap(imagePath);
    if (!pixmap.isNull()) {
        backgroundImage = pixmap;
        backgroundImagePreserveSize = preserveOriginalSize;
        backgroundType = BackgroundType::Image;
        needsRedraw = true;
        update();
    }
}

void CanvasWidget::setBackgroundVideo(const QString &videoPath, bool loop)
{
    if (videoPlayer) {
        videoPlayer->stop();
        videoPlayer->setSource(QUrl::fromLocalFile(videoPath));
        videoPlayer->setLoops(loop ? QMediaPlayer::Infinite : 1);
        videoPlayer->play();
        backgroundType = BackgroundType::Video;
        needsRedraw = true;
    }
}

void CanvasWidget::clearBackground()
{
    if (videoPlayer) {
        videoPlayer->stop();
        videoPlayer->setSource(QUrl());
    }
    backgroundType = BackgroundType::SolidColor;
    backgroundColor = Qt::black;
    needsRedraw = true;
    update();
}

void CanvasWidget::showOverlay(const QString &text)
{
    overlayText = text;
    overlayReference.clear();
    needsRedraw = true;
    update();
}

void CanvasWidget::showOverlayWithReference(const QString &reference, const QString &text)
{
    overlayReference = reference;
    overlayText = text;
    needsRedraw = true;
    update();
}

void CanvasWidget::clearOverlay()
{
    overlayText.clear();
    overlayReference.clear();
    notesVisibleFlag = false;  // Hide notes overlay but keep content in editor
    needsRedraw = true;
    update();
}

void CanvasWidget::setNotesHtml(const QString &html)
{
    notesHtml = html;
    if (notesVisibleFlag) {
        needsRedraw = true;
        update();
    }
}

void CanvasWidget::setNotesVisible(bool visible)
{
    if (notesVisibleFlag == visible) {
        return;
    }
    notesVisibleFlag = visible;
    needsRedraw = true;
    update();
}

void CanvasWidget::setOverlayConfig(const OverlayConfig &config)
{
    overlayConfig = config;
    needsRedraw = true;
    update();
}

QImage CanvasWidget::getFrame() const
{
    QImage image(size(), QImage::Format_RGB32);
    QPainter painter(&image);
    
    const_cast<CanvasWidget*>(this)->drawBackground(painter);
    
    const bool hasMainOverlayText = !overlayText.isEmpty() || !overlayReference.isEmpty();
    const bool hasNotes = notesVisibleFlag && !notesHtml.trimmed().isEmpty();

    if (hasMainOverlayText || hasNotes) {
        const_cast<CanvasWidget*>(this)->drawOverlay(painter);
    }
    
    return image;
}

QImage CanvasWidget::getNotesImage() const
{
    const int baseW = 1920;
    const int baseH = 1080;

    QImage image(baseW, baseH, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    const bool hasNotes = notesVisibleFlag && !notesHtml.trimmed().isEmpty();
    if (!hasNotes) {
        return image;
    }

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QTextDocument notesDoc;
    notesDoc.setDocumentMargin(0);
    // Notes formatting comes solely from the editor HTML
    notesDoc.setHtml(notesHtml);
    notesDoc.setTextWidth(baseW);

    qreal docHeight = notesDoc.size().height();
    int h = qMin(static_cast<int>(std::ceil(docHeight)), baseH);

    QRectF textRect(0, 0, baseW, docHeight);

    painter.save();
    painter.setClipRect(QRectF(0, 0, baseW, h));
    notesDoc.drawContents(&painter, QRectF(0, 0, textRect.width(), h));
    painter.restore();

    return image;
}

void CanvasWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    const bool hasMainOverlayText = !overlayText.isEmpty() || !overlayReference.isEmpty();
    const bool hasNotes = notesVisibleFlag && !notesHtml.trimmed().isEmpty();

    // Fast path: when we are just showing a video background (e.g. local media
    // playback) with no Bible/song text and no notes overlay, draw the video
    // frame directly to the widget. This avoids an extra 1920x1080 offscreen
    // render + rescale, which can make playback feel choppy.
    if (!hasNotes && !hasMainOverlayText && backgroundType == BackgroundType::Video) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        drawBackground(painter);
        return;
    }

    if (hasNotes) {
        // Render notes at a fixed logical resolution (1920x1080) and then
        // scale onto the widget. This keeps the visual relationship between
        // Projection Notes and the Projection Canvas consistent at any
        // preview size, while the underlying document/font sizes remain
        // unchanged.
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // Draw background using widget coordinates
        drawBackground(painter);

        QImage notesImage = getNotesImage();
        if (!notesImage.isNull()) {
            QImage scaled = notesImage.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int x = (width() - scaled.width()) / 2;
            int y = (height() - scaled.height()) / 2;
            painter.drawImage(x, y, scaled);
        }

        return;
    }

    // For Bible verses and song lyrics, continue to render to 1920x1080
    // and scale to widget size for consistent quality.
    QImage renderImage(1920, 1080, QImage::Format_RGB32);
    QPainter renderPainter(&renderImage);
    renderPainter.setRenderHint(QPainter::Antialiasing);
    renderPainter.setRenderHint(QPainter::TextAntialiasing);
    renderPainter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Draw background at 1920x1080
    renderPainter.setCompositionMode(QPainter::CompositionMode_Source);
    switch (backgroundType) {
        case BackgroundType::SolidColor:
            renderPainter.fillRect(0, 0, 1920, 1080, backgroundColor);
            break;
        case BackgroundType::Image:
            if (!backgroundImage.isNull()) {
                if (backgroundImagePreserveSize) {
                    QSize pixSize = backgroundImage.size();
                    QPixmap pixmapToDraw = backgroundImage;
                    if (pixSize.width() > 1920 || pixSize.height() > 1080) {
                        pixmapToDraw = backgroundImage.scaled(1920, 1080, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        pixSize = pixmapToDraw.size();
                    }
                    int x = (1920 - pixSize.width()) / 2;
                    int y = (1080 - pixSize.height()) / 2;
                    renderPainter.drawPixmap(x, y, pixmapToDraw);
                } else {
                    QPixmap scaled = backgroundImage.scaled(1920, 1080, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                    renderPainter.drawPixmap(0, 0, scaled);
                }
            } else {
                renderPainter.fillRect(0, 0, 1920, 1080, Qt::black);
            }
            break;
        case BackgroundType::Video:
            if (!currentVideoFrame.isNull()) {
                QImage scaled = currentVideoFrame.scaled(1920, 1080, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                renderPainter.drawImage(0, 0, scaled);
            } else {
                renderPainter.fillRect(0, 0, 1920, 1080, Qt::black);
            }
            break;
        case BackgroundType::None:
            renderPainter.fillRect(0, 0, 1920, 1080, Qt::transparent);
            break;
    }
    
    // Draw overlay at 1920x1080
    renderPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (hasMainOverlayText) {
        if (overlayText.trimmed().isEmpty() && overlayReference.trimmed().isEmpty()) {
            // Skip
        } else {
            const int w = static_cast<int>(1920 * 0.9);
            const int maxH = static_cast<int>(1080 * 0.85);
            const int x = (1920 - w) / 2;
            const int innerWidth = w - overlayConfig.padding * 2;
            
            QTextDocument mainDoc, refDoc;
            mainDoc.setDocumentMargin(0);
            refDoc.setDocumentMargin(0);
            
            QFont scaledMainFont = overlayConfig.font;
            QFont scaledRefFont = overlayConfig.referenceFont;
            scaledRefFont.setItalic(false);

            QString mainText = overlayConfig.textUppercase ? overlayText.toUpper() : overlayText;
            QString refText = overlayConfig.referenceUppercase ? overlayReference.toUpper() : overlayReference;

            qreal mainHeight = 0.0;
            if (!mainText.trimmed().isEmpty()) {
                populateDocument(mainDoc, mainText, scaledMainFont, overlayConfig.textColor, overlayConfig.alignment, true, overlayConfig.textLineSpacingFactor);
                mainDoc.setTextWidth(innerWidth);
                mainHeight = mainDoc.size().height();
            }
            
            qreal refHeight = 0.0;
            Qt::Alignment refAlignment = overlayConfig.referenceAlignment;
            if (overlayConfig.separateReferenceArea) {
                refAlignment = Qt::AlignCenter;
            }
            int refInnerWidthDraw = innerWidth;
            const int refMaxWidth = static_cast<int>(1920 * 0.7);
            if (!refText.trimmed().isEmpty()) {
                populateDocument(refDoc, refText, scaledRefFont, overlayConfig.referenceColor, refAlignment, false, 1.0);
                if (overlayConfig.separateReferenceArea) {
                    int refMaxInner = refMaxWidth - overlayConfig.padding * 2;
                    if (refMaxInner <= 0) {
                        refMaxInner = innerWidth;
                    }
                    refInnerWidthDraw = qMin(static_cast<int>(std::ceil(refDoc.idealWidth())), refMaxInner);
                    if (refInnerWidthDraw <= 0) {
                        refInnerWidthDraw = refMaxInner;
                    }
                    refDoc.setTextWidth(refInnerWidthDraw);
                } else {
                    refDoc.setTextWidth(innerWidth);
                    refInnerWidthDraw = innerWidth;
                }
                refHeight = refDoc.size().height();
            }
            
            if (overlayConfig.separateReferenceArea && !overlayReference.trimmed().isEmpty()) {
                // Draw main text in central box
                const qreal spacing = (!overlayText.trimmed().isEmpty()) ? 10.0 : 0.0;
                qreal mainBlockHeight = mainHeight + ((overlayText.trimmed().isEmpty()) ? 0.0 : spacing);
                int totalHeightMain = overlayConfig.padding * 2 + static_cast<int>(std::ceil(mainBlockHeight));
                int hMain = qMin(totalHeightMain, maxH);
                
                int y;
                switch (overlayConfig.verticalPosition) {
                    case VerticalPosition::Top:
                        y = 10;
                        break;
                    case VerticalPosition::Bottom:
                        y = 1080 - hMain - 5;
                        break;
                    case VerticalPosition::Center:
                    default:
                        y = (1080 - hMain) / 2;
                        break;
                }
                
                QRectF overlayRect(x, y, w, hMain);
                QColor textFill = overlayConfig.textHighlightEnabled
                    ? overlayConfig.textHighlightColor
                    : overlayConfig.backgroundColor;

                renderPainter.save();
                renderPainter.translate(overlayRect.left() + overlayConfig.padding,
                                        overlayRect.top() + overlayConfig.padding);
                renderPainter.setOpacity(overlayConfig.opacity);
                bool drawBorder = overlayConfig.textBorderEnabled && overlayConfig.textLineSpacingFactor == 1.0;
                drawDocumentHighlightBackground(renderPainter, mainDoc, overlayConfig, textFill, drawBorder, overlayConfig.textBorderThickness);
                renderPainter.setOpacity(1.0);
                mainDoc.drawContents(&renderPainter, QRectF(0, 0, innerWidth, mainHeight));
                renderPainter.restore();
                
                // Draw reference in separate bottom-centered area
                if (!overlayReference.trimmed().isEmpty()) {
                    const int refPadding = overlayConfig.padding;
                    int refWidth = qMin(w, refMaxWidth); // nearly full width highlight
                    int refInnerWidth = refWidth - refPadding * 2;
                    if (refInnerWidth <= 0) {
                        refInnerWidth = innerWidth;
                    }
                    int refX = (1920 - refWidth) / 2;
                    int refHeightBox = refPadding * 2 + static_cast<int>(std::ceil(refHeight));
                    int refY = 1080 - refHeightBox - 50; // position near bottom
                    if (refY < y + overlayRect.height() + 20) {
                        refY = y + overlayRect.height() + 20;
                    }
                    QRectF refOverlayRect(refX, refY, refWidth, refHeightBox);
                    renderPainter.setOpacity(overlayConfig.opacity);
                    if (overlayConfig.referenceHighlightEnabled) {
                        QColor refFill = overlayConfig.referenceHighlightColor;
                        fillRoundedRect(renderPainter, refOverlayRect, refFill, overlayConfig.referenceHighlightCornerRadius);
                    }
                    renderPainter.setOpacity(1.0);
                    refDoc.setTextWidth(refInnerWidth);
                    QRectF refRect(refOverlayRect.left() + refPadding, refOverlayRect.top() + refPadding, refInnerWidth, refHeight);
                    renderPainter.save();
                    renderPainter.translate(refRect.topLeft());
                    refDoc.drawContents(&renderPainter, QRectF(0, 0, refRect.width(), refRect.height()));
                    renderPainter.restore();
                }
            } else {
                const qreal spacing = (!overlayReference.trimmed().isEmpty() && !overlayText.trimmed().isEmpty()) ? 10.0 : 0.0;
                qreal totalTextHeight = mainHeight + refHeight + spacing;
                int totalHeight = overlayConfig.padding * 2 + static_cast<int>(std::ceil(totalTextHeight));
                int h = qMin(totalHeight, maxH);
                
                int y;
                switch (overlayConfig.verticalPosition) {
                    case VerticalPosition::Top:
                        y = 10;
                        break;
                    case VerticalPosition::Bottom:
                        y = 1080 - h - 5;
                        break;
                    case VerticalPosition::Center:
                    default:
                        y = (1080 - h) / 2;
                        break;
                }
                
                QRectF overlayRect(x, y, w, h);
                renderPainter.setOpacity(overlayConfig.opacity);
                renderPainter.setOpacity(1.0);
                
                const int textLeft = overlayRect.left() + overlayConfig.padding;
                qreal currentY = overlayRect.top() + overlayConfig.padding;
                
                if (!overlayReference.trimmed().isEmpty() && overlayConfig.referencePosition == ReferencePosition::Above) {
                    QRectF refRect(textLeft, currentY, innerWidth, refHeight);
                    if (overlayConfig.referenceHighlightEnabled) {
                        QRectF backgroundRect = refRect.adjusted(-overlayConfig.padding / 2.0, -overlayConfig.padding / 2.0,
                                                                 overlayConfig.padding / 2.0, overlayConfig.padding / 2.0);
                        fillRoundedRect(renderPainter, backgroundRect, overlayConfig.referenceHighlightColor,
                                        overlayConfig.referenceHighlightCornerRadius);
                    }
                    renderPainter.save();
                    renderPainter.translate(refRect.topLeft());
                    refDoc.drawContents(&renderPainter, QRectF(0, 0, refRect.width(), refRect.height()));
                    renderPainter.restore();
                    currentY += refHeight + spacing;
                }
                
                if (!overlayText.trimmed().isEmpty()) {
                    QRectF mainRect(textLeft, currentY, innerWidth, mainHeight);
                    QColor textFill = overlayConfig.textHighlightEnabled
                        ? overlayConfig.textHighlightColor
                        : overlayConfig.backgroundColor;
                    renderPainter.save();
                    renderPainter.translate(mainRect.topLeft());
                    bool drawBorder = overlayConfig.textBorderEnabled && overlayConfig.textLineSpacingFactor == 1.0;
                    drawDocumentHighlightBackground(renderPainter, mainDoc, overlayConfig, textFill, drawBorder, overlayConfig.textBorderThickness);
                    mainDoc.drawContents(&renderPainter, QRectF(0, 0, mainRect.width(), mainRect.height()));
                    renderPainter.restore();
                    currentY += mainHeight;
                }
                
                if (!overlayReference.trimmed().isEmpty() && overlayConfig.referencePosition == ReferencePosition::Below) {
                    if (!overlayText.trimmed().isEmpty()) {
                        currentY += spacing;
                    }
                    QRectF refRect(textLeft, currentY, innerWidth, refHeight);
                    if (overlayConfig.referenceHighlightEnabled) {
                        QRect backgroundRect = refRect.toAlignedRect().adjusted(-overlayConfig.padding / 2, -overlayConfig.padding / 2,
                                                                                overlayConfig.padding / 2, overlayConfig.padding / 2);
                        renderPainter.setBrush(overlayConfig.referenceHighlightColor);
                        renderPainter.setPen(Qt::NoPen);
                        renderPainter.drawRect(backgroundRect);
                    }
                    renderPainter.save();
                    renderPainter.translate(refRect.topLeft());
                    refDoc.drawContents(&renderPainter, QRectF(0, 0, refRect.width(), refRect.height()));
                    renderPainter.restore();
                }
            }
        }
    }
    
    // Scale and draw to widget
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(rect(), renderImage);

    // Optional fade overlay from previous frame
    if (fadeActive && !fadePrevFrame.isNull()) {
        painter.save();
        painter.setOpacity(1.0 - fadeProgress);
        painter.drawPixmap(rect(), fadePrevFrame);
        painter.restore();
    } else if (!fadeActive && !fadePrevFrame.isNull()) {
        fadePrevFrame = QPixmap();
    }
}

void CanvasWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Rescale background image if needed
    if (backgroundType == BackgroundType::Image && !backgroundImage.isNull()) {
        // Will be rescaled on next paint
        needsRedraw = true;
    }
    
    update();
}

void CanvasWidget::onVideoFrameChanged(const QVideoFrame &frame)
{
    if (frame.isValid()) {
        QVideoFrame clonedFrame(frame);
        if (clonedFrame.map(QVideoFrame::ReadOnly)) {
            currentVideoFrame = clonedFrame.toImage();
            clonedFrame.unmap();
            needsRedraw = true;
            update();
        }
    }
}

void CanvasWidget::drawBackground(QPainter &painter)
{
    switch (backgroundType) {
        case BackgroundType::SolidColor:
            painter.fillRect(rect(), backgroundColor);
            break;
            
        case BackgroundType::Image:
            if (!backgroundImage.isNull()) {
                QPixmap scaled = backgroundImage.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                int x = (width() - scaled.width()) / 2;
                int y = (height() - scaled.height()) / 2;
                painter.drawPixmap(x, y, scaled);
            } else {
                painter.fillRect(rect(), Qt::black);
            }
            break;
            
        case BackgroundType::Video:
            if (!currentVideoFrame.isNull()) {
                QImage scaled = currentVideoFrame.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                int x = (width() - scaled.width()) / 2;
                int y = (height() - scaled.height()) / 2;
                painter.drawImage(x, y, scaled);
            } else {
                painter.fillRect(rect(), Qt::black);
            }
            break;
            
        case BackgroundType::None:
            painter.fillRect(rect(), Qt::black);
            break;
    }
}

void CanvasWidget::drawOverlay(QPainter &painter)
{
    const bool hasMainOverlayText = !overlayText.trimmed().isEmpty() || !overlayReference.trimmed().isEmpty();
    const bool hasNotes = notesVisibleFlag && !notesHtml.trimmed().isEmpty();

    if (!hasMainOverlayText && !hasNotes) {
        return;
    }

    painter.save();

    if (hasNotes) {
        const int w = width();
        const int maxH = height();
        const int x = 0;
        const int innerWidth = w;
        if (innerWidth <= 0) {
            painter.restore();
            return;
        }

        QTextDocument notesDoc;
        notesDoc.setDocumentMargin(0);
        // Do NOT apply overlayConfig.font here; notes formatting comes solely from the editor HTML
        notesDoc.setHtml(notesHtml);
        notesDoc.setTextWidth(innerWidth);

        qreal docHeight = notesDoc.size().height();
        qreal totalTextHeight = docHeight;
        int totalHeight = static_cast<int>(std::ceil(totalTextHeight));
        int h = qMin(totalHeight, maxH);

        // Top-left aligned, full width of widget
        int y = 0;

        QRectF overlayRect(x, y, w, h);

        QRectF textRect(overlayRect.left(),
                        overlayRect.top(),
                        innerWidth,
                        docHeight);

        painter.save();
        painter.translate(textRect.topLeft());
        notesDoc.drawContents(&painter, QRectF(0, 0, textRect.width(), textRect.height()));
        painter.restore();
    } else if (hasMainOverlayText) {
        const int w = static_cast<int>(width() * 0.9);
        const int maxH = static_cast<int>(height() * 0.85);
        const int x = (width() - w) / 2;
        const int innerWidth = w - overlayConfig.padding * 2;
        if (innerWidth <= 0) {
            painter.restore();
            return;
        }
        // Prepare QTextDocuments for main text and reference
        QTextDocument mainDoc;
        mainDoc.setDocumentMargin(0);
        QTextDocument refDoc;
        refDoc.setDocumentMargin(0);

        QFont scaledMainFont = overlayConfig.font;
        QFont scaledRefFont = overlayConfig.referenceFont;
        scaledRefFont.setItalic(false);

        QString mainText = overlayConfig.textUppercase ? overlayText.toUpper() : overlayText;
        QString refText = overlayConfig.referenceUppercase ? overlayReference.toUpper() : overlayReference;

        qreal mainHeight = 0.0;
        if (!mainText.trimmed().isEmpty()) {
            populateDocument(mainDoc,
                             mainText,
                             scaledMainFont,
                             overlayConfig.textColor,
                             overlayConfig.alignment,
                             true,
                             overlayConfig.textLineSpacingFactor);
            mainDoc.setTextWidth(innerWidth);
            mainHeight = mainDoc.size().height();
        }

        qreal refHeight = 0.0;
        if (!refText.trimmed().isEmpty()) {
            populateDocument(refDoc,
                             refText,
                             scaledRefFont,
                             overlayConfig.referenceColor,
                             overlayConfig.referenceAlignment,
                             false,
                             1.0);
            refDoc.setTextWidth(innerWidth);
            refHeight = refDoc.size().height();
        }

        const qreal spacing = (!overlayReference.trimmed().isEmpty() && !overlayText.trimmed().isEmpty()) ? 10.0 : 0.0;
        qreal totalTextHeight = mainHeight + refHeight + spacing;
        int totalHeight = overlayConfig.padding * 2 + static_cast<int>(std::ceil(totalTextHeight));
        int h = qMin(totalHeight, maxH);

        int y;
        switch (overlayConfig.verticalPosition) {
            case VerticalPosition::Top:
                y = 10;
                break;
            case VerticalPosition::Bottom:
                y = height() - h - 5;
                break;
            case VerticalPosition::Center:
            default:
                y = (height() - h) / 2;
                break;
        }

        QRect overlayRect(x, y, w, h);

        painter.setOpacity(overlayConfig.opacity);
        painter.fillRect(overlayRect, overlayConfig.backgroundColor);
        painter.setOpacity(1.0);

        const int textLeft = overlayRect.left() + overlayConfig.padding;
        qreal currentY = overlayRect.top() + overlayConfig.padding;

        if (!overlayReference.trimmed().isEmpty() && overlayConfig.referencePosition == ReferencePosition::Above) {
            QRectF refRect(textLeft, currentY, innerWidth, refHeight);
            painter.save();
            painter.translate(refRect.topLeft());
            refDoc.drawContents(&painter, QRectF(0, 0, refRect.width(), refRect.height()));
            painter.restore();
            currentY += refHeight + spacing;
        }

        if (!overlayText.trimmed().isEmpty()) {
            QRectF mainRect(textLeft, currentY, innerWidth, mainHeight);
            painter.save();
            painter.translate(mainRect.topLeft());
            mainDoc.drawContents(&painter, QRectF(0, 0, mainRect.width(), mainRect.height()));
            painter.restore();
            currentY += mainHeight;
        }

        if (!overlayReference.trimmed().isEmpty() && overlayConfig.referencePosition == ReferencePosition::Below) {
            if (!overlayText.trimmed().isEmpty()) {
                currentY += spacing;
            }
            QRectF refRect(textLeft, currentY, innerWidth, refHeight);
            painter.save();
            painter.translate(refRect.topLeft());
            refDoc.drawContents(&painter, QRectF(0, 0, refRect.width(), refRect.height()));
            painter.restore();
        }
    }

    painter.restore();
}

void CanvasWidget::populateDocument(QTextDocument &doc,
                                    const QString &text,
                                    const QFont &font,
                                    const QColor &color,
                                    Qt::Alignment alignment,
                                    bool italicizeBracketContent,
                                    qreal lineSpacingFactor)
{
    doc.clear();
    QTextOption option;
    option.setAlignment(alignment);
    option.setWrapMode(QTextOption::WordWrap);
    doc.setDefaultTextOption(option);

    QTextCursor cursor(&doc);
    QTextBlockFormat blockFormat;
    blockFormat.setAlignment(alignment);
    blockFormat.setLineHeight(qRound(qBound(50.0, lineSpacingFactor * 100.0, 400.0)), QTextBlockFormat::ProportionalHeight);
    cursor.setBlockFormat(blockFormat);

    QTextCharFormat format;
    format.setFont(font);
    format.setForeground(color);

    QString buffer;
    auto flushBuffer = [&]() {
        if (buffer.isEmpty()) {
            return;
        }
        cursor.insertText(buffer, format);
        buffer.clear();
    };

    bool italicActive = false;
    for (const QChar &ch : text) {
        if (ch == '\r') {
            continue;
        }
        if (italicizeBracketContent && ch == '[') {
            flushBuffer();
            italicActive = true;
            format.setFontItalic(true);
            continue;
        }
        if (italicizeBracketContent && ch == ']') {
            flushBuffer();
            italicActive = false;
            format.setFontItalic(false);
            continue;
        }
        if (ch == '\n') {
            flushBuffer();
            cursor.insertBlock(blockFormat);
            continue;
        }
        buffer.append(ch);
    }
    flushBuffer();

    if (italicActive) {
        format.setFontItalic(false);
    }
}
