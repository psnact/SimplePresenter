#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QFont>
#include <QColor>
#include <QRect>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QAudioOutput>
#include <QTextDocument>

class BackgroundRenderer;
class QTimer;

enum class BackgroundType {
    None,
    SolidColor,
    Image,
    Video
};

enum class VerticalPosition {
    Top,
    Center,
    Bottom
};

enum class ReferencePosition {
    Above,  // Reference above main text
    Below   // Reference below main text
};

struct OverlayConfig {
    QRect geometry;
    QFont font;
    QFont referenceFont;  // Separate font for Bible references
    QColor textColor;
    QColor referenceColor;  // Separate color for Bible references
    QColor backgroundColor;
    Qt::Alignment alignment;  // Left, Center, Right for main text
    Qt::Alignment referenceAlignment;  // Left, Center, Right for Bible reference
    VerticalPosition verticalPosition;  // Top, Center, Bottom
    ReferencePosition referencePosition;  // Above or Below main text
    int padding;
    float opacity;
    bool separateReferenceArea;
    bool textHighlightEnabled;
    QColor textHighlightColor;
    bool referenceHighlightEnabled;
    QColor referenceHighlightColor;
    int textHighlightCornerRadius;
    int referenceHighlightCornerRadius;
    bool textUppercase;
    bool referenceUppercase;
    qreal textLineSpacingFactor;
    bool textBorderEnabled;
    int textBorderThickness;
    QColor textBorderColor;
    int textBorderPaddingHorizontal;
    int textBorderPaddingVertical;
    
    OverlayConfig() 
        : geometry(0, 0, 800, 600)
        , alignment(Qt::AlignCenter)
        , referenceAlignment(Qt::AlignCenter)
        , padding(40)
        , opacity(1.0)
        , font("Arial", 48)
        , referenceFont("Arial", 36)
        , textColor(Qt::white)
        , referenceColor(Qt::white)
        , backgroundColor(0, 0, 0, 180)
        , textHighlightEnabled(true)
        , textHighlightColor(0, 0, 0, 180)
        , referenceHighlightEnabled(false)
        , referenceHighlightColor(0, 0, 0, 180)
        , textHighlightCornerRadius(0)
        , referenceHighlightCornerRadius(0)
        , textUppercase(false)
        , referenceUppercase(false)
        , textLineSpacingFactor(1.0)
        , textBorderEnabled(false)
        , textBorderThickness(4)
        , textBorderColor(Qt::white)
        , textBorderPaddingHorizontal(20)
        , textBorderPaddingVertical(20)
    {
        font.setFamily("Arial");
        referenceFont.setFamily("Arial");
        referenceFont.setPointSize(36);
    }
};

class CanvasWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CanvasWidget(QWidget *parent = nullptr);
    ~CanvasWidget();
    
    // Render at 1920x1080 but display scaled
    QSize sizeHint() const override { return QSize(960, 540); }
    QSize minimumSizeHint() const override { return QSize(480, 270); }
    
    // Background control
    void setBackgroundColor(const QColor &color);
    void setBackgroundImage(const QString &imagePath, bool preserveOriginalSize = false);
    void setBackgroundVideo(const QString &videoPath, bool loop = true);
    void clearBackground();
    
    // Overlay control
    void showOverlay(const QString &text);
    void showOverlayWithReference(const QString &reference, const QString &text);
    void clearOverlay();
    bool hasOverlay() const { return !overlayText.isEmpty(); }
    
    // Overlay configuration
    void setOverlayConfig(const OverlayConfig &config);
    OverlayConfig getOverlayConfig() const { return overlayConfig; }
    
    // Get rendered frame for streaming
    QImage getFrame() const;

    // Render only the notes overlay (no background) to an image so it can be
    // mirrored into external outputs like the OBS overlay.
    QImage getNotesImage() const;

    // Notes overlay (separate from main overlay text/reference)
    void setNotesHtml(const QString &html);
    void setNotesVisible(bool visible);
    bool notesVisible() const { return notesVisibleFlag; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void drawOverlay(QPainter &painter);  // Made protected for subclasses

private slots:
    void onVideoFrameChanged(const QVideoFrame &frame);

private:
    void drawBackground(QPainter &painter);
    void updateVideoFrame();

protected:
    void setBackgroundImageWithFade(const QString &imagePath, int durationMs = 250);

    BackgroundType backgroundType;
    QColor backgroundColor;
    QPixmap backgroundImage;
    bool backgroundImagePreserveSize;
    QMediaPlayer *videoPlayer;
    QVideoSink *videoSink;
    QAudioOutput *audioOutputDevice;
    QImage currentVideoFrame;
    
    QString overlayText;
    QString overlayReference;  // Separate reference text for Bible verses
    OverlayConfig overlayConfig;

    // Optional rich-text notes overlay drawn instead of main overlay when visible
    QString notesHtml;
    bool notesVisibleFlag;
    
protected:
    QString getOverlayText() const { return overlayText; }
    QString getOverlayReference() const { return overlayReference; }
    QMediaPlayer *mediaPlayer() const { return videoPlayer; }
    BackgroundType currentBackgroundType() const { return backgroundType; }
    QAudioOutput *audioOutput() const { return audioOutputDevice; }

    QPixmap cachedFrame;
    bool needsRedraw;

    // Fade transition support
    QTimer *fadeTimer;
    bool fadeActive;
    qreal fadeProgress;
    int fadeDurationMs;
    QPixmap fadePrevFrame;

private:
    static void populateDocument(QTextDocument &doc,
                                 const QString &text,
                                 const QFont &font,
                                 const QColor &color,
                                 Qt::Alignment alignment,
                                 bool italicizeBracketContent,
                                 qreal lineSpacingFactor = 1.0);
};

#endif // CANVASWIDGET_H
