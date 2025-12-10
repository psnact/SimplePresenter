#include "ProjectionCanvas.h"
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QPixmap>
#include <QResizeEvent>
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QWebEnginePage>
#include <QRegularExpression>
#include <QVariant>
#endif
#include <QUrl>
#include <QUrlQuery>

ProjectionCanvas::ProjectionCanvas(QWidget *parent)
    : CanvasWidget(parent)
    , useSeparateBackgrounds(false)
    , currentContent(ContentType::Verse)
    , playingMediaVideo(false)
    , backgroundInitialized(false)
    , activeMediaPath()
    , startupSplashActive(false)
    , nextImageFade(false)
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    , youtubePlayer(nullptr)
#endif
{
    loadSettings(false);
    bibleOverlayConfig = getOverlayConfig();
    songOverlayConfig = bibleOverlayConfig;
    clearOverlay();
    activeMediaPath.clear();
    playingMediaVideo = false;
    mediaOverrideActive = false;
    notesModeActive = false;
    backgroundInitialized = false;

    // Projection canvases are display-only; ignore all direct user input.
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);

    setStartupSplash();
}

ProjectionCanvas::~ProjectionCanvas()
{
    saveSettings();
}

QMediaPlayer *ProjectionCanvas::mediaPlayer() const
{
    return CanvasWidget::mediaPlayer();
}

bool ProjectionCanvas::isMediaVideoActive() const
{
    return playingMediaVideo;
}

QAudioOutput *ProjectionCanvas::mediaAudioOutput() const
{
    return CanvasWidget::audioOutput();
}

void ProjectionCanvas::stopMediaPlaybackIfMatches(const QString &path)
{
    if (activeMediaPath.isEmpty()) {
        return;
    }

    const QString normalizedRequested = QDir::fromNativeSeparators(path);
    const QString normalizedActive = QDir::fromNativeSeparators(activeMediaPath);
    if (!normalizedRequested.isEmpty() && normalizedRequested.compare(normalizedActive, Qt::CaseInsensitive) != 0) {
        return;
    }

    if (QMediaPlayer *player = mediaPlayer()) {
        player->stop();
        player->setSource(QUrl());
    }
    activeMediaPath.clear();
    setMediaVideoActive(false);
    applyStoredBackground();
}

void ProjectionCanvas::setMediaVideoActive(bool active)
{
    if (playingMediaVideo == active) {
        return;
    }

    playingMediaVideo = active;
    if (playingMediaVideo) {
        emit mediaVideoPlaybackStarted();
    } else {
        emit mediaVideoPlaybackStopped();
    }
}

void ProjectionCanvas::showBibleVerse(const QString &reference, const QString &text)
{
    if (startupSplashActive) {
        startupSplashActive = false;
        refreshCurrentBackground();
    }
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    if (youtubePlayer) {
        youtubePlayer->hide();
        youtubePlayer->stop();
        youtubePlayer->setHtml(QStringLiteral("<html><body style=\"margin:0;background:black;\"></body></html>"),
                               QUrl(QStringLiteral("http://localhost/")));
    }
#endif
    formatBibleText(reference, text);
}

void ProjectionCanvas::showLyrics(const QString &songTitle, const QString &lyrics)
{
    if (startupSplashActive) {
        startupSplashActive = false;
        refreshCurrentBackground();
    }
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    if (youtubePlayer) {
        youtubePlayer->hide();
        youtubePlayer->stop();
        youtubePlayer->setHtml(QStringLiteral("<html><body style=\"margin:0;background:black;\"></body></html>"),
                               QUrl(QStringLiteral("http://localhost/")));
    }
#endif
    formatLyricsText(songTitle, lyrics);
}

ProjectionCanvas::ContentBackground ProjectionCanvas::makeBackgroundFromSettings(QSettings &settings, const QString &prefix, const ContentBackground &fallback)
{
    ContentBackground background = fallback;

    const auto key = [&prefix](const QString &suffix) {
        return prefix.isEmpty() ? suffix : prefix + QLatin1Char('/') + suffix;
    };

    const QString typeStr = settings.value(key("backgroundType"), QStringLiteral("color")).toString();

    if (typeStr == QLatin1String("video")) {
        background.type = BackgroundType::Video;
        background.video = settings.value(key("video"), fallback.video).toString();
        background.loop = settings.value(key("loop"), fallback.loop).toBool();
        if (background.video.isEmpty() || !QFileInfo::exists(background.video)) {
            background = fallback;
        }
    } else if (typeStr == QLatin1String("image")) {
        background.type = BackgroundType::Image;
        background.image = settings.value(key("image"), fallback.image).toString();
        background.loop = fallback.loop;
        if (background.image.isEmpty() || !QFileInfo::exists(background.image)) {
            background = fallback;
        }
    } else if (typeStr == QLatin1String("none")) {
        background.type = BackgroundType::None;
        background.loop = fallback.loop;
    } else {
        background.type = BackgroundType::SolidColor;
        background.color = settings.value(key("backgroundColor"), fallback.color).value<QColor>();
        background.loop = fallback.loop;
    }

    return background;
}

bool ProjectionCanvas::backgroundsEqual(const ContentBackground &lhs, const ContentBackground &rhs)
{
    if (lhs.type != rhs.type) {
        return false;
    }

    switch (lhs.type) {
        case BackgroundType::SolidColor:
            return lhs.color == rhs.color;
        case BackgroundType::Image:
            return lhs.image == rhs.image;
        case BackgroundType::Video:
            return lhs.video == rhs.video && lhs.loop == rhs.loop;
        case BackgroundType::None:
            return true;
    }

    return false;
}

void ProjectionCanvas::ContentBackground::apply(CanvasWidget *canvas) const
{
    switch (type) {
        case BackgroundType::SolidColor:
            canvas->setBackgroundColor(color);
            break;
        case BackgroundType::Image:
            canvas->setBackgroundImage(image);
            break;
        case BackgroundType::Video:
            canvas->setBackgroundVideo(video, loop);
            break;
        case BackgroundType::None:
            canvas->clearBackground();
            break;
    }
}

void ProjectionCanvas::applyBackground(const ContentBackground &background)
{
    background.apply(this);
    storedBackground = background;
}

void ProjectionCanvas::storeCurrentBackground()
{
    storedBackground = currentBackground;
}

void ProjectionCanvas::refreshCurrentBackground()
{
    ContentBackground desired;
    if (notesModeActive) {
        // Notes always use the dedicated notes background from settings,
        // which itself falls back to the Bible background if not
        // configured. This makes notes independent of the "Use separate
        // backgrounds" option that only applies to songs.
        desired = notesBackground;
    } else {
        desired = (currentContent == ContentType::Verse) ? verseBackground : songBackground;
    }
    currentBackground = desired;
    if (!mediaOverrideActive && (!backgroundInitialized || !backgroundsEqual(storedBackground, desired))) {
        applyBackground(currentBackground);
        backgroundInitialized = true;
    }
}

void ProjectionCanvas::setStartupSplash()
{
    const QString splashPath(QStringLiteral(":/Splashscreen.png"));
    QPixmap pixmap(splashPath);
    if (!pixmap.isNull()) {
        setBackgroundImage(splashPath, true);
        startupSplashActive = true;
    } else {
        startupSplashActive = false;
        refreshCurrentBackground();
    }
}

void ProjectionCanvas::formatBibleText(const QString &reference, const QString &text)
{
    currentContent = ContentType::Verse;
    if (playingMediaVideo) {
        setMediaVideoActive(false);
    }
    if (mediaOverrideActive) {
        applyStoredBackground();
    } else {
        refreshCurrentBackground();
    }
    setOverlayConfig(bibleOverlayConfig);
    showOverlayWithReference(reference, text);
}

void ProjectionCanvas::formatLyricsText(const QString &songTitle, const QString &lyrics)
{
    currentContent = ContentType::Song;
    if (playingMediaVideo) {
        setMediaVideoActive(false);
    }
    if (mediaOverrideActive) {
        applyStoredBackground();
    } else {
        refreshCurrentBackground();
    }
    setOverlayConfig(songOverlayConfig);
    showOverlay(lyrics);
}

void ProjectionCanvas::showMedia(const QString &path, bool isVideo)
{
    if (startupSplashActive) {
        startupSplashActive = false;
        refreshCurrentBackground();
    }
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    if (youtubePlayer) {
        youtubePlayer->hide();
        youtubePlayer->stop();
        youtubePlayer->setHtml(QStringLiteral("<html><body style=\"margin:0;background:black;\"></body></html>"),
                               QUrl(QStringLiteral("http://localhost/")));
    }
#endif
    storeCurrentBackground();

    mediaOverrideActive = true;
    clearOverlay();

    if (isVideo) {
        setBackgroundVideo(path, true);
        setMediaVideoActive(true);
        activeMediaPath = path;
    } else {
        if (nextImageFade) {
            setBackgroundImageWithFade(path, 250);
        } else {
            setBackgroundImage(path);
        }
        setMediaVideoActive(false);
        activeMediaPath.clear();
        nextImageFade = false;
    }
}

void ProjectionCanvas::setNextImageFade(bool enabled)
{
    nextImageFade = enabled;
}

void ProjectionCanvas::applyStoredBackground()
{
    mediaOverrideActive = false;
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    if (youtubePlayer) {
        youtubePlayer->hide();
        youtubePlayer->stop();
        youtubePlayer->setHtml(QStringLiteral("<html><body style=\"margin:0;background:black;\"></body></html>"),
                               QUrl(QStringLiteral("http://localhost/")));
    }
#endif
    applyBackground(storedBackground);
    refreshCurrentBackground();
    if (playingMediaVideo) {
        setMediaVideoActive(false);
    }
    activeMediaPath.clear();
}

void ProjectionCanvas::setBibleBackground(const QString &type,
                                          const QColor &color,
                                          const QString &imagePath,
                                          const QString &videoPath,
                                          bool loopVideo)
{
    auto createBackground = [&](ContentBackground &target) {
        if (type == QLatin1String("video")) {
            target.type = BackgroundType::Video;
            target.video = videoPath;
            target.loop = loopVideo;
            target.image.clear();
        } else if (type == QLatin1String("image")) {
            target.type = BackgroundType::Image;
            target.image = imagePath;
            target.video.clear();
            target.loop = loopVideo;
        } else if (type == QLatin1String("none")) {
            target.type = BackgroundType::None;
            target.image.clear();
            target.video.clear();
            target.loop = loopVideo;
        } else {
            target.type = BackgroundType::SolidColor;
            target.color = color;
            target.image.clear();
            target.video.clear();
            target.loop = loopVideo;
        }
    };

    createBackground(verseBackground);
    if (!useSeparateBackgrounds) {
        songBackground = verseBackground;
    }
    if (currentContent == ContentType::Verse || !useSeparateBackgrounds) {
        refreshCurrentBackground();
    }
}

void ProjectionCanvas::setSongBackground(const QString &type,
                                         const QColor &color,
                                         const QString &imagePath,
                                         const QString &videoPath,
                                         bool loopVideo)
{
    auto createBackground = [&](ContentBackground &target) {
        if (type == QLatin1String("video")) {
            target.type = BackgroundType::Video;
            target.video = videoPath;
            target.loop = loopVideo;
            target.image.clear();
        } else if (type == QLatin1String("image")) {
            target.type = BackgroundType::Image;
            target.image = imagePath;
            target.video.clear();
            target.loop = loopVideo;
        } else if (type == QLatin1String("none")) {
            target.type = BackgroundType::None;
            target.image.clear();
            target.video.clear();
            target.loop = loopVideo;
        } else {
            target.type = BackgroundType::SolidColor;
            target.color = color;
            target.image.clear();
            target.video.clear();
            target.loop = loopVideo;
        }
    };

    createBackground(songBackground);
    if (!useSeparateBackgrounds) {
        verseBackground = songBackground;
    }
    if (currentContent == ContentType::Song || !useSeparateBackgrounds) {
        refreshCurrentBackground();
    }
}

void ProjectionCanvas::setSeparateBackgroundsEnabled(bool enabled)
{
    useSeparateBackgrounds = enabled;
    if (!enabled) {
        songBackground = verseBackground;
    }
    refreshCurrentBackground();
}

void ProjectionCanvas::loadSettings(bool applyBackground)
{
    QSettings settings("SimplePresenter", "SimplePresenter");

    settings.beginGroup("ProjectionCanvas");

    OverlayConfig bibleConfig = bibleOverlayConfig;

    bibleConfig.geometry = settings.value("overlayGeometry", bibleConfig.geometry).toRect();

    QFont defaultBibleFont("Arial", 48);
    bibleConfig.font = settings.value("font", defaultBibleFont).value<QFont>();
    bibleConfig.font.setBold(false);

    QFont defaultRefFont("Arial", 36);
    bibleConfig.referenceFont = settings.value("referenceFont", defaultRefFont).value<QFont>();
    bibleConfig.referenceFont.setBold(false);

    bibleConfig.textColor = settings.value("textColor", bibleConfig.textColor).value<QColor>();
    bibleConfig.referenceColor = settings.value("referenceColor", bibleConfig.referenceColor).value<QColor>();

    QColor bibleBg = settings.value("backgroundColor", bibleConfig.backgroundColor).value<QColor>();
    int bibleOpacity = settings.value("backgroundOpacity", -1).toInt();
    if (bibleOpacity >= 0) {
        bibleBg.setAlpha(qRound(qBound(0, bibleOpacity, 100) * 255.0 / 100.0));
    } else if (settings.value("backgroundTransparent", false).toBool() && bibleBg.alpha() != 0) {
        bibleBg.setAlpha(0);
    } else if (bibleBg.alpha() == 0) {
        bibleBg.setAlpha(180);
    }
    bibleConfig.backgroundColor = bibleBg;

    bibleConfig.textHighlightEnabled = settings.value("textHighlightEnabled", bibleConfig.textHighlightEnabled).toBool();
    bibleConfig.textHighlightColor = settings.value("textHighlightColor", bibleConfig.textHighlightColor).value<QColor>();
    bibleConfig.referenceHighlightEnabled = settings.value("referenceHighlightEnabled", bibleConfig.referenceHighlightEnabled).toBool();
    bibleConfig.referenceHighlightColor = settings.value("referenceHighlightColor", bibleConfig.referenceHighlightColor).value<QColor>();
    bibleConfig.textHighlightCornerRadius = qBound(0, settings.value("textHighlightCornerRadius", bibleConfig.textHighlightCornerRadius).toInt(), 200);
    bibleConfig.referenceHighlightCornerRadius = qBound(0, settings.value("referenceHighlightCornerRadius", bibleConfig.referenceHighlightCornerRadius).toInt(), 200);
    bibleConfig.textUppercase = settings.value("textUppercase", bibleConfig.textUppercase).toBool();
    bibleConfig.referenceUppercase = settings.value("referenceUppercase", bibleConfig.referenceUppercase).toBool();

    bibleConfig.alignment = Qt::Alignment(settings.value("alignment", int(bibleConfig.alignment)).toInt());
    bibleConfig.referenceAlignment = Qt::Alignment(settings.value("referenceAlignment", int(bibleConfig.referenceAlignment)).toInt());
    bibleConfig.padding = settings.value("padding", bibleConfig.padding).toInt();
    bibleConfig.opacity = settings.value("opacity", bibleConfig.opacity).toFloat();
    bibleConfig.verticalPosition = static_cast<VerticalPosition>(settings.value("verticalPosition", int(bibleConfig.verticalPosition)).toInt());
    bibleConfig.referencePosition = static_cast<ReferencePosition>(settings.value("referencePosition", int(bibleConfig.referencePosition)).toInt());
    bibleConfig.separateReferenceArea = settings.value("separateReferenceArea", bibleConfig.separateReferenceArea).toBool();

    OverlayConfig songConfig = bibleConfig;
    songConfig.referenceHighlightEnabled = false;
    songConfig.separateReferenceArea = false;

    QFont defaultSongFont = songConfig.font;
    songConfig.font = settings.value("song/font", defaultSongFont).value<QFont>();
    songConfig.font.setBold(false);
    if (songConfig.font.pointSize() <= 0) {
        songConfig.font.setPointSize(defaultSongFont.pointSize());
    }
    songConfig.textColor = settings.value("song/textColor", songConfig.textColor).value<QColor>();

    QColor songBg = settings.value("song/backgroundColor", songConfig.backgroundColor).value<QColor>();
    int songOpacity = settings.value("song/backgroundOpacity", qRound(songBg.alpha() * 100.0 / 255.0)).toInt();
    songBg.setAlpha(qRound(qBound(0, songOpacity, 100) * 255.0 / 100.0));
    songConfig.backgroundColor = songBg;
    songConfig.textHighlightEnabled = settings.value("song/textHighlightEnabled", songConfig.textHighlightEnabled).toBool();
    songConfig.textHighlightColor = settings.value("song/textHighlightColor", songConfig.textHighlightColor).value<QColor>();
    int songCornerRadius = qBound(0, settings.value("song/backgroundCornerRadius", int(songConfig.textHighlightCornerRadius)).toInt(), 200);
    songConfig.textHighlightCornerRadius = songCornerRadius;
    int spacingPercent = settings.value("song/lineSpacingPercent", 120).toInt();
    spacingPercent = qBound(100, spacingPercent, 200);
    songConfig.textLineSpacingFactor = spacingPercent / 100.0;
    songConfig.textBorderEnabled = settings.value("song/borderEnabled", false).toBool();
    songConfig.textBorderThickness = qBound(1, settings.value("song/borderThickness", 4).toInt(), 20);
    songConfig.textBorderColor = settings.value("song/borderColor", songConfig.textBorderColor).value<QColor>();
    songConfig.textBorderPaddingHorizontal = qBound(0, settings.value("song/borderPaddingHorizontal", songConfig.textBorderPaddingHorizontal).toInt(), 400);
    songConfig.textBorderPaddingVertical = qBound(0, settings.value("song/borderPaddingVertical", songConfig.textBorderPaddingVertical).toInt(), 400);
    if (songConfig.textLineSpacingFactor > 1.0) {
        songConfig.textBorderEnabled = false;
    }
    songConfig.alignment = Qt::Alignment(settings.value("song/alignment", int(songConfig.alignment)).toInt());
    songConfig.verticalPosition = static_cast<VerticalPosition>(settings.value("song/verticalPosition", int(songConfig.verticalPosition)).toInt());
    songConfig.textUppercase = settings.value("song/textUppercase", songConfig.textUppercase).toBool();

    settings.endGroup();

    bibleOverlayConfig = bibleConfig;
    songOverlayConfig = songConfig;

    if (currentContent == ContentType::Song) {
        setOverlayConfig(songOverlayConfig);
    } else {
        setOverlayConfig(bibleOverlayConfig);
    }

    settings.beginGroup("Backgrounds");

    ContentBackground baseFallback;
    baseFallback.type = BackgroundType::SolidColor;
    baseFallback.color = QColor(Qt::black);
    baseFallback.loop = true;

    ContentBackground generalBackground = makeBackgroundFromSettings(settings, QString(), baseFallback);
    ContentBackground verseBg = makeBackgroundFromSettings(settings, QStringLiteral("verse"), generalBackground);

    bool separate = settings.value("separateSongBackground", false).toBool();
    ContentBackground songBgConfig = separate
        ? makeBackgroundFromSettings(settings, QStringLiteral("song"), verseBg)
        : verseBg;

    // Notes background falls back to verse background if not configured
    ContentBackground notesBg = makeBackgroundFromSettings(settings, QStringLiteral("notes"), verseBg);

    settings.endGroup();

    verseBackground = verseBg;
    songBackground = songBgConfig;
    // If separate backgrounds are disabled, both songs and notes should
    // share the Bible background. When enabled, notes use their own
    // configured background (falling back to Bible if not set).
    useSeparateBackgrounds = separate;
    if (!useSeparateBackgrounds) {
        songBackground = verseBackground;
        notesBackground = verseBackground;
    } else {
        notesBackground = notesBg;
    }

    if (applyBackground && !startupSplashActive) {
        refreshCurrentBackground();
    } else {
        backgroundInitialized = false;
        if (notesModeActive) {
            currentBackground = useSeparateBackgrounds ? notesBackground : verseBackground;
        } else {
            currentBackground = (currentContent == ContentType::Verse) ? verseBackground : songBackground;
        }
        storedBackground = currentBackground;
    }
}

void ProjectionCanvas::saveSettings()
{
    if (currentContent == ContentType::Song) {
        songOverlayConfig = getOverlayConfig();
    } else {
        bibleOverlayConfig = getOverlayConfig();
    }

    QSettings settings("SimplePresenter", "SimplePresenter");

    settings.beginGroup("ProjectionCanvas");

    const OverlayConfig &bibleConfig = bibleOverlayConfig;
    settings.setValue("overlayGeometry", bibleConfig.geometry);
    settings.setValue("font", bibleConfig.font);
    settings.setValue("referenceFont", bibleConfig.referenceFont);
    settings.setValue("textColor", bibleConfig.textColor);
    settings.setValue("referenceColor", bibleConfig.referenceColor);
    settings.setValue("textUppercase", bibleConfig.textUppercase);
    settings.setValue("referenceUppercase", bibleConfig.referenceUppercase);
    QColor bibleBg = bibleConfig.backgroundColor;
    int bibleOpacity = qRound(bibleBg.alpha() * 100.0 / 255.0);
    settings.setValue("backgroundColor", bibleBg);
    settings.setValue("backgroundOpacity", bibleOpacity);
    settings.setValue("backgroundTransparent", bibleOpacity == 0);
    settings.setValue("textHighlightEnabled", bibleConfig.textHighlightEnabled);
    settings.setValue("textHighlightColor", bibleConfig.textHighlightColor);
    settings.setValue("referenceHighlightEnabled", bibleConfig.referenceHighlightEnabled);
    settings.setValue("referenceHighlightColor", bibleConfig.referenceHighlightColor);
    settings.setValue("textHighlightCornerRadius", bibleConfig.textHighlightCornerRadius);
    settings.setValue("referenceHighlightCornerRadius", bibleConfig.referenceHighlightCornerRadius);
    settings.setValue("alignment", int(bibleConfig.alignment));
    settings.setValue("referenceAlignment", int(bibleConfig.referenceAlignment));
    settings.setValue("referencePosition", int(bibleConfig.referencePosition));
    settings.setValue("separateReferenceArea", bibleConfig.separateReferenceArea);
    settings.setValue("padding", bibleConfig.padding);
    settings.setValue("opacity", bibleConfig.opacity);
    settings.setValue("verticalPosition", int(bibleConfig.verticalPosition));

    const OverlayConfig &songConfig = songOverlayConfig;
    settings.setValue("song/font", songConfig.font);
    settings.setValue("song/textUppercase", songConfig.textUppercase);
    settings.setValue("song/textColor", songConfig.textColor);
    QColor songBg = songConfig.backgroundColor;
    int songOpacity = qRound(songBg.alpha() * 100.0 / 255.0);
    settings.setValue("song/backgroundColor", songBg);
    settings.setValue("song/backgroundOpacity", songOpacity);
    settings.setValue("song/textHighlightEnabled", songConfig.textHighlightEnabled);
    settings.setValue("song/textHighlightColor", songConfig.textHighlightColor);
    settings.setValue("song/backgroundCornerRadius", songConfig.textHighlightCornerRadius);
    settings.setValue("song/lineSpacingPercent", qRound(songConfig.textLineSpacingFactor * 100.0));
    settings.setValue("song/borderEnabled", songConfig.textBorderEnabled);
    settings.setValue("song/borderThickness", songConfig.textBorderThickness);
    settings.setValue("song/borderColor", songConfig.textBorderColor);
    settings.setValue("song/borderPaddingHorizontal", songConfig.textBorderPaddingHorizontal);
    settings.setValue("song/borderPaddingVertical", songConfig.textBorderPaddingVertical);
    settings.setValue("song/alignment", int(songConfig.alignment));
    settings.setValue("song/verticalPosition", int(songConfig.verticalPosition));

    settings.endGroup();

    settings.beginGroup("Backgrounds");
    settings.setValue("separateSongBackground", useSeparateBackgrounds);

    auto saveBackground = [&](const QString &prefix, const ContentBackground &background) {
        auto key = [&prefix](const QString &suffix) {
            return prefix.isEmpty() ? suffix : prefix + '/' + suffix;
        };

        QString type;
        switch (background.type) {
        case BackgroundType::SolidColor:
            type = QStringLiteral("color");
            break;
        case BackgroundType::Image:
            type = QStringLiteral("image");
            break;
        case BackgroundType::Video:
            type = QStringLiteral("video");
            break;
        case BackgroundType::None:
            type = QStringLiteral("none");
            break;
        }

        settings.setValue(key("backgroundType"), type);
        settings.setValue(key("backgroundColor"), background.color);
        settings.setValue(key("image"), background.image);
        settings.setValue(key("video"), background.video);
        settings.setValue(key("loop"), background.loop);
    };

    saveBackground(QString(), verseBackground);
    saveBackground(QStringLiteral("verse"), verseBackground);
    saveBackground(QStringLiteral("song"), songBackground);
    saveBackground(QStringLiteral("notes"), notesBackground);

    settings.endGroup();
}

void ProjectionCanvas::setNotesModeActive(bool active)
{
    if (notesModeActive == active) {
        return;
    }

    notesModeActive = active;

    if (!mediaOverrideActive) {
        refreshCurrentBackground();
    }
}

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
void ProjectionCanvas::showYouTubeVideo(const QString &url)
{
    if (startupSplashActive) {
        startupSplashActive = false;
        refreshCurrentBackground();
    }

    QUrl sourceUrl(url);

    const QString host = sourceUrl.host();
    QString videoId;
    QString startTime;

    if (host.contains("youtube.com")) {
        const QString path = sourceUrl.path();
        if (path.startsWith("/embed/")) {
            // /embed/VIDEO_ID
            videoId = path.mid(QStringLiteral("/embed/").size());
            QUrlQuery query(sourceUrl);
            startTime = query.queryItemValue("start");
            if (startTime.isEmpty()) {
                startTime = query.queryItemValue("t");
            }
        } else if (path == "/watch") {
            QUrlQuery query(sourceUrl);
            videoId   = query.queryItemValue("v");
            startTime = query.queryItemValue("t");
        } else if (path.startsWith("/shorts/")) {
            // Handle YouTube Shorts URLs: /shorts/VIDEO_ID
            videoId = path.mid(QStringLiteral("/shorts/").size());
            QUrlQuery query(sourceUrl);
            startTime = query.queryItemValue("t");
        }
    } else if (host.contains("youtu.be")) {
        videoId = sourceUrl.path().mid(1);
        QUrlQuery query(sourceUrl);
        startTime = query.queryItemValue("t");
    }

    // Fallback: try to extract videoId and t= directly from the raw URL string
    if (videoId.isEmpty()) {
        const QString raw = url;
        QRegularExpression reId(QStringLiteral("[?&]v=([^&#]+)"));
        QRegularExpressionMatch mId = reId.match(raw);
        if (mId.hasMatch()) {
            videoId = mId.captured(1);
        }
        if (videoId.isEmpty()) {
            QRegularExpression reShort(QStringLiteral("youtu\\.be/([^?&#/]+)"));
            QRegularExpressionMatch mShort = reShort.match(raw);
            if (mShort.hasMatch()) {
                videoId = mShort.captured(1);
            }
        }
        if (startTime.isEmpty()) {
            QRegularExpression reT(QStringLiteral("[?&]t=([0-9]+)"));
            QRegularExpressionMatch mT = reT.match(raw);
            if (mT.hasMatch()) {
                startTime = mT.captured(1);
            }
        }
    }

    if (youtubePlayer) {
        youtubePlayer->hide();
        youtubePlayer->deleteLater();
        youtubePlayer = nullptr;
    }

    youtubePlayer = new QWebEngineView(this);
    youtubePlayer->settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    youtubePlayer->setStyleSheet("background: black;");
    youtubePlayer->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    youtubePlayer->setFocusPolicy(Qt::NoFocus);

    storeCurrentBackground();
    mediaOverrideActive = true;
    clearOverlay();

    // Default to the original URL if we cannot parse a video ID
    if (videoId.isEmpty()) {
        youtubePlayer->setUrl(sourceUrl);
    } else {
        bool ok = false;
        int startSeconds = startTime.toInt(&ok);
        if (!ok || startSeconds < 0) {
            startSeconds = 0;
        }

        const QString html = QStringLiteral(
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            "<style>html,body{margin:0;padding:0;overflow:hidden;background:black;width:100%;height:100%;}"
            "#player{position:absolute;top:0;left:0;width:100%!important;height:100%!important;border:0;}"
            "</style>"
            "<script src=\"https://www.youtube.com/iframe_api\"></script>"
            "</head><body>"
            "<iframe id=\"player\" src=\"https://www.youtube.com/embed/%1?enablejsapi=1&autoplay=1&rel=0&modestbranding=1&playsinline=1&start=%2\" "
            "frameborder=\"0\" allow=\"accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share\" allowfullscreen></iframe>"
            "<script>"
            "var spPlayer=null;"
            "function onYouTubeIframeAPIReady(){"
            "  try {"
            "    spPlayer=new YT.Player('player');"
            "  } catch(e) { console.error('YouTube API init failed',e); }"
            "}"
            "function spSeekToRatio(r){"
            "  try {"
            "    if(!spPlayer||typeof spPlayer.getDuration!=='function'||typeof spPlayer.seekTo!=='function') return;"
            "    var d=spPlayer.getDuration();"
            "    if(!d||d<=0) return;"
            "    if(r<0) r=0; if(r>1) r=1;"
            "    spPlayer.seekTo(d*r,true);"
            "  } catch(e) { console.error('spSeekToRatio failed',e); }"
            "}"
            "function spPlayPause(play){"
            "  try {"
            "    if(!spPlayer) return;"
            "    if(play){ if(spPlayer.playVideo) spPlayer.playVideo(); }"
            "    else { if(spPlayer.pauseVideo) spPlayer.pauseVideo(); }"
            "  } catch(e) { console.error('spPlayPause failed',e); }"
            "}"
            "function spSetVolume(level){"
            "  try {"
            "    if(!spPlayer||typeof spPlayer.setVolume!=='function') return;"
            "    if(level<0) level=0; if(level>100) level=100;"
            "    spPlayer.setVolume(level);"
            "  } catch(e) { console.error('spSetVolume failed',e); }"
            "}"
            "function spGetPositionRatio(){"
            "  try {"
            "    if(!spPlayer||typeof spPlayer.getDuration!=='function'||typeof spPlayer.getCurrentTime!=='function') return -1;"
            "    var d=spPlayer.getDuration();"
            "    if(!d||d<=0) return -1;"
            "    var t=spPlayer.getCurrentTime();"
            "    if(t<0) t=0;"
            "    return t/d;"
            "  } catch(e) { console.error('spGetPositionRatio failed',e); return -1; }"
            "}"
            "</script>"
            "</body></html>"
        ).arg(videoId).arg(startSeconds);

        // Use an HTTP base URL so the iframe's origin is http://localhost
        youtubePlayer->setHtml(html, QUrl(QStringLiteral("http://localhost/")));
    }

    youtubePlayer->resize(size());
    youtubePlayer->show();
    youtubePlayer->raise();

    // Stop local media
    if (playingMediaVideo) {
        setMediaVideoActive(false);
    }
    activeMediaPath.clear();
}

bool ProjectionCanvas::isYouTubeActive() const
{
    return youtubePlayer && youtubePlayer->isVisible();
}

void ProjectionCanvas::seekYouTubeByRatio(double ratio)
{
    if (!youtubePlayer) {
        return;
    }

    if (ratio < 0.0) {
        ratio = 0.0;
    } else if (ratio > 1.0) {
        ratio = 1.0;
    }

    if (QWebEnginePage *page = youtubePlayer->page()) {
        const QString js = QStringLiteral(
            "if (typeof spSeekToRatio === 'function') spSeekToRatio(%1);"
        ).arg(QString::number(ratio, 'f', 6));
        page->runJavaScript(js);
    }
}

void ProjectionCanvas::queryYouTubePositionRatio(const std::function<void(double)> &callback)
{
    if (!youtubePlayer || !callback) {
        return;
    }

    if (QWebEnginePage *page = youtubePlayer->page()) {
        const QString js = QStringLiteral(
            "(function(){"
            "  try {"
            "    if (typeof spGetPositionRatio !== 'function') return -1;"
            "    return spGetPositionRatio();"
            "  } catch (e) { console.error('queryYouTubePositionRatio failed', e); return -1; }"
            "})();"
        );

        page->runJavaScript(js, [callback](const QVariant &result) {
            bool ok = false;
            double ratio = result.toDouble(&ok);
            if (!ok || ratio < 0.0) {
                callback(-1.0);
                return;
            }
            if (ratio < 0.0) {
                ratio = 0.0;
            } else if (ratio > 1.0) {
                ratio = 1.0;
            }
            callback(ratio);
        });
    }
}

void ProjectionCanvas::setYouTubeMuted(bool muted)
{
    if (!youtubePlayer) {
        return;
    }

    if (QWebEnginePage *page = youtubePlayer->page()) {
        page->setAudioMuted(muted);
    }
}

void ProjectionCanvas::setYouTubeVolume(double volume01)
{
    if (!youtubePlayer) {
        return;
    }

    if (volume01 < 0.0) {
        volume01 = 0.0;
    } else if (volume01 > 1.0) {
        volume01 = 1.0;
    }

    const int level = qBound(0, static_cast<int>(volume01 * 100.0 + 0.5), 100);

    if (QWebEnginePage *page = youtubePlayer->page()) {
        const QString js = QStringLiteral(
            "if (typeof spSetVolume === 'function') spSetVolume(%1);"
        ).arg(level);
        page->runJavaScript(js);
    }
}

void ProjectionCanvas::playPauseYouTube(bool play)
{
    if (!youtubePlayer) {
        return;
    }

    if (QWebEnginePage *page = youtubePlayer->page()) {
        const QString js = QStringLiteral(
            "if (typeof spPlayPause === 'function') spPlayPause(%1);"
        ).arg(play ? QStringLiteral("true") : QStringLiteral("false"));
        page->runJavaScript(js);
    }
}

void ProjectionCanvas::resizeEvent(QResizeEvent *event)
{
    CanvasWidget::resizeEvent(event);
    if (youtubePlayer) {
        youtubePlayer->resize(event->size());
    }
}
#endif
