#ifndef PROJECTIONCANVAS_H
#define PROJECTIONCANVAS_H

#include "CanvasWidget.h"
#include <QColor>
#include <QString>
#include <QPixmap>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QTimer>
#include <QMovie>
#include <QGraphicsOpacityEffect>
#include <functional>

class QSettings;
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
class QWebEngineView;
#endif

class ProjectionCanvas : public CanvasWidget
{
    Q_OBJECT

public:
    explicit ProjectionCanvas(QWidget *parent = nullptr);
    ~ProjectionCanvas();
    
    // Content display methods
    void showBibleVerse(const QString &reference, const QString &text);
    void showLyrics(const QString &songTitle, const QString &lyrics);
    void showMedia(const QString &path, bool isVideo);
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    void showYouTubeVideo(const QString &url);
    bool isYouTubeActive() const;
    void seekYouTubeByRatio(double ratio);
    void playPauseYouTube(bool play);
    void setYouTubeVolume(double volume01);
    void setYouTubeMuted(bool muted);
    void queryYouTubePositionRatio(const std::function<void(double)> &callback);
#endif
    void setNextImageFade(bool enabled);

    QMediaPlayer *mediaPlayer() const;
    bool isMediaVideoActive() const;
    QAudioOutput *mediaAudioOutput() const;
    void stopMediaPlaybackIfMatches(const QString &path);

    // Background management
    void setBibleBackground(const QString &type,
                            const QColor &color,
                            const QString &imagePath,
                            const QString &videoPath,
                            bool loopVideo);
    void setSongBackground(const QString &type,
                           const QColor &color,
                           const QString &imagePath,
                           const QString &videoPath,
                           bool loopVideo);
    void setSeparateBackgroundsEnabled(bool enabled);

    // Activate or deactivate the notes background (used when Projection Notes are shown)
    void setNotesModeActive(bool active);

    // Load projection-specific settings
    void loadSettings(bool applyBackground = true);
    void saveSettings();

signals:
    void mediaVideoPlaybackStarted();
    void mediaVideoPlaybackStopped();

protected:
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    void resizeEvent(QResizeEvent *event) override;
#endif

private:
    enum class ContentType {
        Verse,
        Song
    };

    struct ContentBackground {
        BackgroundType type = BackgroundType::SolidColor;
        QColor color = Qt::black;
        QString image;
        QString video;
        bool loop = true;

        void apply(CanvasWidget *canvas) const;
    };

    void formatBibleText(const QString &reference, const QString &text);
    void formatLyricsText(const QString &songTitle, const QString &lyrics);
    void applyStoredBackground();
    void applyBackground(const ContentBackground &background);
    void storeCurrentBackground();
    void refreshCurrentBackground();
    void setStartupSplash();
    static ContentBackground makeBackgroundFromSettings(QSettings &settings, const QString &prefix, const ContentBackground &fallback);
    static bool backgroundsEqual(const ContentBackground &lhs, const ContentBackground &rhs);
    void setMediaVideoActive(bool active);

    ContentBackground verseBackground;
    ContentBackground songBackground;
    ContentBackground notesBackground;
    ContentBackground storedBackground;
    ContentBackground currentBackground;
    OverlayConfig bibleOverlayConfig;
    OverlayConfig songOverlayConfig;
    bool useSeparateBackgrounds;
    bool mediaOverrideActive;
    bool notesModeActive;
    ContentType currentContent;
    bool playingMediaVideo;
    bool backgroundInitialized;
    QString activeMediaPath;
    bool startupSplashActive;
    bool nextImageFade;
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    QWebEngineView *youtubePlayer;
#endif
};

#endif // PROJECTIONCANVAS_H
