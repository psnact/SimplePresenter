#include "MainWindow.h"
#include "BiblePanel.h"
#include "SongPanel.h"
#include "MediaPanel.h"
#include "PowerPointPanel.h"
#include "PlaylistPanel.h"
#include "ProjectionCanvas.h"
#include "SettingsDialog.h"
#include "OverlayServer.h"

#include "UpdateChecker.h"
#include <QDesktopServices>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QScreen>
#include <QWindow>
#include <QApplication>
#include <QGuiApplication>
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QToolButton>
#include <QSlider>
#include <QStyle>
#include <QTextEdit>
#include <QFrame>
#include <QFontComboBox>
#include <QComboBox>
#include <QButtonGroup>
#include <QColorDialog>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextListFormat>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include <QTimer>
#include <climits>
#include <QStandardPaths>
#include <QDir>
#include <QImage>
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
#include <QWebEngineView>
#include <QWebEnginePage>
#endif
#include <QPushButton>
#include <QUrl>
#include <QPalette>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QTabBar>
#include <QLineEdit>
#include <QToolTip>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , mainSplitter(nullptr)
    , verticalSplitter(nullptr)
    , contentTabs(nullptr)
    , biblePanel(nullptr)
    , songPanel(nullptr)
    , mediaPanel(nullptr)
    , powerPointPanel(nullptr)
    , playlistPanel(nullptr)
    , projectionCanvas(nullptr)
    , fullscreenProjection(nullptr)
    , notesTabWidget(nullptr)
    , notesEditor(nullptr)
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    , youtubeBrowser(nullptr)
#endif
    , notesShowButton(nullptr)
    , notesPrevPageButton(nullptr)
    , notesNextPageButton(nullptr)
    , notesAddPageButton(nullptr)
    , notesPageJumpEdit(nullptr)
    , notesPageStatusLabel(nullptr)
    , notesPages()
    , currentNotesPageIndex(0)
    , notesLastGoodHtml()
    , notesPageFull(false)
    , overlayServer(nullptr)
    , mediaControlsWidget(nullptr)
    , mediaPlayPauseButton(nullptr)
    , mediaSeekSlider(nullptr)
    , mediaVolumeSlider(nullptr)
    , mediaSeekSliderDragging(false)
    , youtubePlaying(false)
    , youtubePositionTimer(nullptr)
    , settings(nullptr)
{
    setWindowTitle("SimplePresenter");
    resize(1600, 1000);
    
    settings = new QSettings("SimplePresenter", "SimplePresenter", this);
    
    // Initialize overlay server
    overlayServer = new OverlayServer(this);
    if (overlayServer->start(8080)) {
        qDebug() << "Overlay server started on port 8080";
        qDebug() << "OBS Browser Source URL: http://localhost:8080/overlay";
    } else {
        qWarning() << "Failed to start overlay server";
    }

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    connect(overlayServer, &OverlayServer::webSocketClientConnected,
            this, [this]() {
                if (!projectionCanvas) {
                    return;
                }
                if (projectionCanvas->isYouTubeActive()) {
                    QTimer::singleShot(1000, this, [this]() { syncYouTubeOverlayToProjection(); });
                    QTimer::singleShot(3000, this, [this]() { syncYouTubeOverlayToProjection(); });
                } else if (projectionCanvas->isMediaVideoActive()) {
                    QTimer::singleShot(1000, this, [this]() { syncMediaOverlayToProjection(); });
                    QTimer::singleShot(3000, this, [this]() { syncMediaOverlayToProjection(); });
                }
            });
#endif

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    youtubePositionTimer = new QTimer(this);
    youtubePositionTimer->setInterval(250);
    youtubePositionTimer->setSingleShot(false);
    connect(youtubePositionTimer, &QTimer::timeout,
            this, [this]() {
                if (!projectionCanvas || !mediaControlsWidget || !mediaControlsWidget->isVisible() ||
                    mediaSeekSliderDragging || !mediaSeekSlider) {
                    return;
                }
                if (!projectionCanvas->isYouTubeActive()) {
                    return;
                }

                projectionCanvas->queryYouTubePositionRatio([this](double ratio) {
                    if (ratio < 0.0) {
                        return;
                    }
                    if (!mediaSeekSlider || !mediaControlsWidget || !mediaControlsWidget->isVisible() ||
                        mediaSeekSliderDragging) {
                        return;
                    }

                    int sliderValue = qBound(0, static_cast<int>(ratio * 1000.0 + 0.5), 1000);
                    mediaSeekSlider->blockSignals(true);
                    mediaSeekSlider->setValue(sliderValue);
                    mediaSeekSlider->blockSignals(false);
                });
            });
#endif
    
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    loadSettings();

    
    // Initialize UpdateChecker
    updateChecker = new UpdateChecker(this);
    connect(updateChecker, &UpdateChecker::updateAvailable, this, 
        [this](const QString &version, const QString &downloadUrl, const QString &releaseNotes) {
            QString message = QString("A new version (%1) is available!\n\nRelease Notes:\n%2\n\nWould you like to download it?")
                                .arg(version, releaseNotes);
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Update Available", message, QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                QDesktopServices::openUrl(QUrl(downloadUrl));
            }
        });
    connect(updateChecker, &UpdateChecker::noUpdateAvailable, this, [this]() {
        QMessageBox::information(this, "Check for Updates", "You are using the latest version.");
    });
    connect(updateChecker, &UpdateChecker::checkFailed, this, [this](const QString &error) {
        QMessageBox::warning(this, "Update Check Failed", "Failed to check for updates:\n" + error);
    });

    QTimer::singleShot(0, this, [this]() {
        updateProjectionAndNotesAspect();
    });
}

void MainWindow::onCheckForUpdates()
{
    // Replace with your actual raw JSON URL
    // e.g., "https://raw.githubusercontent.com/username/repo/main/update_info.json"
    // For now, using a placeholder that user will need to configure
    updateChecker->checkForUpdates("https://raw.githubusercontent.com/your-username/SimplePresenter/main/update_info.json");
}

MainWindow::~MainWindow()
{
    saveSettings();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // When the projection canvas itself is resized by the layout system
    // (for example when the main window is restored down and the splitter
    // recalculates widths), re-apply the 16:9 aspect ratio based on the
    // canvas' final width.
    if (watched == projectionCanvas && event && event->type() == QEvent::Resize) {
        QTimer::singleShot(0, this, [this]() {
            updateProjectionAndNotesAspect();
        });
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::WindowStateChange) {
        QTimer::singleShot(0, this, [this]() {
            updateProjectionAndNotesAspect();
        });
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // Keep a stable 16:9 aspect ratio for the projection preview canvas
    // and make the notes editor content area exactly the same size (1:1).
    // Use a single-shot timer so this runs after Qt has updated child layouts,
    // which avoids transient wrong sizes when toggling maximize/restore.
    QTimer::singleShot(0, this, [this]() {
        updateProjectionAndNotesAspect();
    });
}

void MainWindow::updateProjectionAndNotesAspect()
{
    if (!projectionCanvas || !notesEditor || !notesPreviewView || !notesProxy) {
        return;
    }

    int canvasWidth = projectionCanvas->width();
    if (canvasWidth > 0) {
        int targetHeight = static_cast<int>(canvasWidth * 9.0 / 16.0);

        if (projectionCanvas->height() != targetHeight) {
            projectionCanvas->setFixedHeight(targetHeight);
        }
        // Keep the graphics view at the same 16:9 size as the projection canvas
        if (notesPreviewView->width() != canvasWidth || notesPreviewView->height() != targetHeight) {
            notesPreviewView->setFixedSize(canvasWidth, targetHeight);
        }

        // Logical page size for notes preview (fixed 16:9 canvas). The
        // QGraphicsView scales this page so that the on-screen preview zooms
        // in or out when the window is resized, without touching the actual
        // notes document or font sizes.
        const QSizeF baseSize(1920.0, 1080.0);
        notesProxy->setPos(0, 0);
        notesEditor->setFixedSize(baseSize.toSize());
        QSizeF viewSize = notesPreviewView->viewport()->size();
        if (viewSize.width() > 0 && viewSize.height() > 0) {
            qreal scaleX = viewSize.width() / baseSize.width();
            qreal scaleY = viewSize.height() / baseSize.height();
            qreal s = qMin(scaleX, scaleY);
            QTransform t;
            t.scale(s, s);
            notesPreviewView->setTransform(t);
        }
    }
}

void MainWindow::updateNotesOverlayAsMedia()
{
    // Legacy function no longer used for OBS; notes are now sent as live
    // HTML to the overlay instead of a captured screenshot.
}

void MainWindow::syncMediaOverlayToProjection()
{
    if (!projectionCanvas || !overlayServer) {
        return;
    }
    if (!projectionCanvas->isMediaVideoActive()) {
        return;
    }

    QMediaPlayer *player = projectionCanvas->mediaPlayer();
    if (!player) {
        return;
    }

    qint64 position = player->position();
    if (position < 0) {
        return;
    }

    overlayServer->broadcastMediaSeek(position);

    bool playing = (player->playbackState() == QMediaPlayer::PlayingState);
    overlayServer->broadcastMediaPlayPause(playing);
}

void MainWindow::syncYouTubeOverlayToProjection()
{
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    if (!projectionCanvas || !overlayServer) {
        return;
    }
    if (!projectionCanvas->isYouTubeActive()) {
        return;
    }

    projectionCanvas->queryYouTubePositionRatio([this](double ratio) {
        if (!overlayServer) {
            return;
        }
        if (ratio < 0.0) {
            return;
        }

        overlayServer->broadcastYouTubeSeek(ratio);
        overlayServer->broadcastYouTubePlayPause(youtubePlaying);
    });
#endif
}

void MainWindow::setupUI()
{
    static const char *modernTabStyle =
        "QTabWidget::pane {"
        "  border-top: 1px solid #d0d0d0;"
        "  border-left: none;"
        "  border-right: none;"
        "  border-bottom: none;"
        "  background: #f3f4f6;"
        "}"
        "QTabBar::tab {"
        "  background: #ffffff;"
        "  color: #555555;"
        "  padding: 6px 18px;"
        "  margin: 6px 4px 0 4px;"
        "  border: 1px solid #d0d0d0;"
        "  border-radius: 14px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #000000;"
        "  color: #ffffff;"
        "  border-color: #000000;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background: #f0f0f0;"
        "}"
        "QTabBar::tab:!selected {"
        "  color: #666666;"
        "}";

    // Create main vertical splitter (top: controls, bottom: previews)
    verticalSplitter = new QSplitter(Qt::Vertical, this);
    
    // Top section: horizontal splitter for controls
    mainSplitter = new QSplitter(Qt::Horizontal, verticalSplitter);
    
    // Left side: Content tabs (Bible, Songs)
    contentTabs = new QTabWidget(mainSplitter);
    contentTabs->setDocumentMode(true);
    contentTabs->tabBar()->setExpanding(false);
    contentTabs->tabBar()->setElideMode(Qt::ElideRight);
    contentTabs->setStyleSheet(QString::fromLatin1(modernTabStyle));
    
    biblePanel = new BiblePanel(contentTabs);
    songPanel = new SongPanel(contentTabs);
    mediaPanel = new MediaPanel(contentTabs);
    powerPointPanel = new PowerPointPanel(contentTabs);

    contentTabs->addTab(biblePanel, "Bible");
    contentTabs->addTab(songPanel, "Songs");
    contentTabs->addTab(mediaPanel, "Media");
    contentTabs->addTab(powerPointPanel, "PowerPoint");
    
    // Right side: Playlist panel
    playlistPanel = new PlaylistPanel(mainSplitter);
    
    mainSplitter->addWidget(contentTabs);
    mainSplitter->addWidget(playlistPanel);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 1);
    
    verticalSplitter->addWidget(mainSplitter);
    
    // Bottom section: Canvas previews
    QWidget *previewWidget = new QWidget(verticalSplitter);
    QHBoxLayout *previewLayout = new QHBoxLayout(previewWidget);
    previewLayout->setContentsMargins(5, 5, 5, 5);
    previewLayout->setSpacing(5);
    
    // Projection preview
    QGroupBox *projectionGroup = new QGroupBox("Projection Preview", previewWidget);
    QVBoxLayout *projectionLayout = new QVBoxLayout(projectionGroup);
    projectionLayout->setContentsMargins(2, 2, 2, 2);
    projectionCanvas = new ProjectionCanvas(projectionGroup);
    projectionCanvas->setMinimumSize(400, 225); // 16:9 aspect ratio
    projectionCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    projectionLayout->addWidget(projectionCanvas, 0, Qt::AlignTop);
    // Track canvas size changes if needed in the future (no-op at the moment)
    projectionCanvas->installEventFilter(this);
    setupMediaControls(projectionLayout);
    previewLayout->addWidget(projectionGroup, 1);

    // Projection notes and YouTube panel (right side of projection canvas)
    notesTabWidget = new QTabWidget(previewWidget);
    notesTabWidget->setDocumentMode(true);
    notesTabWidget->tabBar()->setExpanding(false);
    notesTabWidget->tabBar()->setElideMode(Qt::ElideRight);
    notesTabWidget->setStyleSheet(QString::fromLatin1(modernTabStyle));

    // --- Tab 1: Projection Notes ---
    QWidget *notesPage = new QWidget(notesTabWidget);
    notesPage->setAutoFillBackground(true);
    {
        QPalette pal = notesPage->palette();
        pal.setColor(QPalette::Window, QColor(240, 240, 240));
        notesPage->setPalette(pal);
    }
    QVBoxLayout *notesLayout = new QVBoxLayout(notesPage);
    notesLayout->setContentsMargins(2, 2, 2, 2);
    notesLayout->setSpacing(4);

    QWidget *notesToolbar = new QWidget(notesPage);
    QHBoxLayout *notesToolbarLayout = new QHBoxLayout(notesToolbar);
    // Use tight margins and spacing so the toolbar stays slim.
    notesToolbarLayout->setContentsMargins(2, 0, 2, 0);
    notesToolbarLayout->setSpacing(4);

    auto addNotesSeparator = [notesToolbar, notesToolbarLayout]() {
        QFrame *sep = new QFrame(notesToolbar);
        sep->setFrameShape(QFrame::VLine);
        sep->setFrameShadow(QFrame::Sunken);
        sep->setLineWidth(1);
        sep->setFixedHeight(18);
        notesToolbarLayout->addWidget(sep);
    };

    // Show / hide notes overlay button
    notesShowButton = new QToolButton(notesToolbar);
    notesShowButton->setCheckable(true);
    notesShowButton->setObjectName(QStringLiteral("notesShowButton"));
    notesShowButton->setIconSize(QSize(18, 18));
    QIcon notesEyeIcon;
    QIcon notesEyeOffIcon;
    {
        const int w = 24;
        const int h = 18;
        QPixmap eyePixmap(w, h);
        eyePixmap.fill(Qt::transparent);

        QPainter p(&eyePixmap);
        p.setRenderHint(QPainter::Antialiasing, true);

        // Outer eye shape
        QPen pen(QColor(30, 30, 30));
        pen.setWidth(2);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QRectF outerRect(2.0, 3.0, w - 4.0, h - 6.0);
        p.drawEllipse(outerRect);

        // Pupil
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(30, 30, 30));
        QPointF center(w / 2.0, h / 2.0);
        p.drawEllipse(center, 3.0, 3.0);

        notesEyeIcon = QIcon(eyePixmap);

        QPixmap eyeOffPixmap = eyePixmap;
        QPainter p2(&eyeOffPixmap);
        p2.setRenderHint(QPainter::Antialiasing, true);
        QPen slashPen(QColor(200, 50, 50));
        slashPen.setWidth(2);
        p2.setPen(slashPen);
        p2.setBrush(Qt::NoBrush);
        p2.drawLine(QPointF(4, h - 3), QPointF(w - 4, 3));

        notesEyeOffIcon = QIcon(eyeOffPixmap);
    }
    notesShowButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    notesShowButton->setText(QString());
    notesShowButton->setIcon(notesEyeOffIcon);
    notesShowButton->setToolTip("Show or hide notes on the projection canvas");
    notesToolbarLayout->addWidget(notesShowButton);
    connect(notesShowButton, &QToolButton::toggled, this,
            [this, notesEyeIcon, notesEyeOffIcon](bool checked) {
                if (!notesShowButton) {
                    return;
                }
                notesShowButton->setIcon(checked ? notesEyeIcon : notesEyeOffIcon);
            });
    addNotesSeparator();

    // Font family
    QFontComboBox *notesFontCombo = new QFontComboBox(notesToolbar);
    notesFontCombo->setMaximumWidth(160);
    notesToolbarLayout->addWidget(notesFontCombo);

    // Font size
    QComboBox *notesSizeCombo = new QComboBox(notesToolbar);
    QList<int> sizeOptions = {24, 28, 32, 36, 40, 48, 56, 64, 72};
    for (int size : sizeOptions) {
        notesSizeCombo->addItem(QString::number(size), size);
    }
    notesSizeCombo->setCurrentIndex(2); // 32pt default
    notesSizeCombo->setMaximumWidth(70);
    notesToolbarLayout->addWidget(notesSizeCombo);
    addNotesSeparator();

    const QSize notesIconSize(18, 18);

    auto makeTextIcon = [](const QString &text, const QFont &baseFont) -> QIcon {
        const int iconSize = 24;
        QPixmap pm(iconSize, iconSize);
        pm.fill(Qt::transparent);
        QPainter painter(&pm);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QFont f = baseFont;
        f.setPointSizeF(f.pointSizeF() * 1.6);
        painter.setFont(f);
        painter.setPen(Qt::black);
        painter.drawText(QRect(0, 0, iconSize, iconSize), Qt::AlignCenter, text);
        return QIcon(pm);
    };

    auto makeAlignIcon = [](Qt::Alignment alignment) -> QIcon {
        const int w = 24;
        const int h = 20;
        QPixmap pm(w, h);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(30, 30, 30));

        const int margin = 3;
        const int lineHeight = 3;
        const int spacing = 3;

        // Make the three align icons much more distinct by varying
        // both the starting x and the width of each line, similar
        // to how Microsoft Word draws them.
        for (int i = 0; i < 3; ++i) {
            int y = margin + i * (lineHeight + spacing);
            int lineWidth = w - 2 * margin;
            int x = margin;

            if (alignment == Qt::AlignLeft) {
                // All lines start at the left; top is shortest, bottom is longest.
                if (i == 0) {
                    lineWidth = w - 6 * margin;
                } else if (i == 1) {
                    lineWidth = w - 4 * margin;
                } else {
                    lineWidth = w - 2 * margin;
                }
                x = margin;
            } else if (alignment == Qt::AlignHCenter) {
                // Lines are centered; middle is longest, top/bottom shorter.
                if (i == 1) {
                    lineWidth = w - 4 * margin;
                } else {
                    lineWidth = w - 6 * margin;
                }
                x = (w - lineWidth) / 2;
            } else if (alignment == Qt::AlignRight) {
                // All lines end at the right; top is shortest, bottom is longest.
                if (i == 0) {
                    lineWidth = w - 6 * margin;
                } else if (i == 1) {
                    lineWidth = w - 4 * margin;
                } else {
                    lineWidth = w - 2 * margin;
                }
                x = w - margin - lineWidth;
            }

            p.drawRect(x, y, lineWidth, lineHeight);
        }

        return QIcon(pm);
    };

    auto makeBulletListIcon = []() -> QIcon {
        const int w = 24;
        const int h = 20;
        QPixmap pm(w, h);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);

        const int margin = 3;
        const int bulletRadius = 3;
        const int lineHeight = 3;
        const int spacing = 3;

        for (int i = 0; i < 3; ++i) {
            int cy = margin + i * (lineHeight + spacing) + lineHeight / 2;
            // Bullet
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(30, 30, 30));
            p.drawEllipse(QPointF(margin + bulletRadius, cy), bulletRadius, bulletRadius);
            // Line
            int x = margin + 2 * bulletRadius + 3;
            int y = cy - lineHeight / 2;
            int lineWidth = w - x - margin;
            p.drawRect(x, y, lineWidth, lineHeight);
        }

        return QIcon(pm);
    };

    auto makeNumberListIcon = []() -> QIcon {
        const int w = 24;
        const int h = 20;
        QPixmap pm(w, h);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);

        const int margin = 3;
        const int lineHeight = 3;
        const int spacing = 3;
        QFont f = p.font();
        f.setPointSizeF(f.pointSizeF() * 0.8);
        p.setFont(f);

        for (int i = 0; i < 3; ++i) {
            int yTop = margin + i * (lineHeight + spacing);
            QRect numberRect(margin, yTop - 1, 10, lineHeight + 4);
            p.setPen(Qt::black);
            p.drawText(numberRect, Qt::AlignLeft | Qt::AlignVCenter,
                       QString::number(i + 1) + ".");

            int x = numberRect.right() + 2;
            int y = yTop;
            int lineWidth = w - x - margin;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(30, 30, 30));
            p.drawRect(x, y, lineWidth, lineHeight);
        }

        return QIcon(pm);
    };

    auto makeHighlightIcon = []() -> QIcon {
        const int w = 24;
        const int h = 20;
        QPixmap pm(w, h);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);

        // Yellow highlight bar at the bottom
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 0));
        p.drawRect(3, h - 7, w - 6, 5);

        // Letter "A" above, similar to Word's highlight icon
        QFont f = p.font();
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::black);
        p.drawText(QRect(0, 0, w, h - 4), Qt::AlignCenter, QStringLiteral("A"));

        return QIcon(pm);
    };

    // Bold / Italic / Underline
    QToolButton *notesBoldButton = new QToolButton(notesToolbar);
    notesBoldButton->setCheckable(true);
    notesBoldButton->setIconSize(notesIconSize);
    QFont boldFont = notesBoldButton->font();
    boldFont.setBold(true);
    notesBoldButton->setIcon(makeTextIcon(QStringLiteral("B"), boldFont));
    notesToolbarLayout->addWidget(notesBoldButton);

    QToolButton *notesItalicButton = new QToolButton(notesToolbar);
    notesItalicButton->setCheckable(true);
    notesItalicButton->setIconSize(notesIconSize);
    QFont italicFont = notesItalicButton->font();
    italicFont.setItalic(true);
    notesItalicButton->setIcon(makeTextIcon(QStringLiteral("I"), italicFont));
    notesToolbarLayout->addWidget(notesItalicButton);

    QToolButton *notesUnderlineButton = new QToolButton(notesToolbar);
    notesUnderlineButton->setCheckable(true);
    notesUnderlineButton->setIconSize(notesIconSize);
    QFont underlineFont = notesUnderlineButton->font();
    underlineFont.setUnderline(true);
    notesUnderlineButton->setIcon(makeTextIcon(QStringLiteral("U"), underlineFont));
    notesToolbarLayout->addWidget(notesUnderlineButton);

    // Text color
    QToolButton *notesColorButton = new QToolButton(notesToolbar);
    notesColorButton->setText("A");
    notesColorButton->setToolTip("Text color");
    notesColorButton->setObjectName(QStringLiteral("notesColorButton"));
    notesToolbarLayout->addWidget(notesColorButton);

    // Highlight color (with dropdown menu for clear option)
    QToolButton *notesHighlightButton = new QToolButton(notesToolbar);
    notesHighlightButton->setIconSize(notesIconSize);
    notesHighlightButton->setToolTip("Highlight color");
    notesHighlightButton->setIcon(makeHighlightIcon());

    QMenu *notesHighlightMenu = new QMenu(notesToolbar);
    QAction *chooseHighlightAction = notesHighlightMenu->addAction("Highlight color...");
    QAction *clearHighlightAction = notesHighlightMenu->addAction("Remove highlight");
    notesHighlightButton->setMenu(notesHighlightMenu);
    notesHighlightButton->setPopupMode(QToolButton::InstantPopup);

    notesToolbarLayout->addWidget(notesHighlightButton);
    addNotesSeparator();

    // Alignment buttons
    QToolButton *alignLeftButton = new QToolButton(notesToolbar);
    alignLeftButton->setCheckable(true);
    alignLeftButton->setIconSize(notesIconSize);
    alignLeftButton->setIcon(makeAlignIcon(Qt::AlignLeft));
    alignLeftButton->setToolTip("Align left");

    QToolButton *alignCenterButton = new QToolButton(notesToolbar);
    alignCenterButton->setCheckable(true);
    alignCenterButton->setIconSize(notesIconSize);
    alignCenterButton->setIcon(makeAlignIcon(Qt::AlignHCenter));
    alignCenterButton->setToolTip("Align center");

    QToolButton *alignRightButton = new QToolButton(notesToolbar);
    alignRightButton->setCheckable(true);
    alignRightButton->setIconSize(notesIconSize);
    alignRightButton->setIcon(makeAlignIcon(Qt::AlignRight));
    alignRightButton->setToolTip("Align right");

    QButtonGroup *alignGroup = new QButtonGroup(notesToolbar);
    alignGroup->setExclusive(true);
    alignGroup->addButton(alignLeftButton);
    alignGroup->addButton(alignCenterButton);
    alignGroup->addButton(alignRightButton);
    alignLeftButton->setChecked(true);

    notesToolbarLayout->addWidget(alignLeftButton);
    notesToolbarLayout->addWidget(alignCenterButton);
    notesToolbarLayout->addWidget(alignRightButton);
    addNotesSeparator();

    // Bullet and numbered list
    QToolButton *bulletListButton = new QToolButton(notesToolbar);
    bulletListButton->setIconSize(notesIconSize);
    bulletListButton->setToolTip("Bulleted list");
    bulletListButton->setIcon(makeBulletListIcon());
    notesToolbarLayout->addWidget(bulletListButton);

    QToolButton *numberListButton = new QToolButton(notesToolbar);
    numberListButton->setIconSize(notesIconSize);
    numberListButton->setToolTip("Numbered list");
    numberListButton->setIcon(makeNumberListIcon());
    notesToolbarLayout->addWidget(numberListButton);
    addNotesSeparator();

    // Push following controls (page navigation) to the right side
    notesToolbarLayout->addStretch();

    QLabel *pagesLabel = new QLabel("Pages", notesToolbar);
    notesToolbarLayout->addWidget(pagesLabel);

    notesPrevPageButton = new QToolButton(notesToolbar);
    notesPrevPageButton->setArrowType(Qt::LeftArrow);
    notesPrevPageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    notesPrevPageButton->setToolTip("Previous notes page");
    notesToolbarLayout->addWidget(notesPrevPageButton);

    notesNextPageButton = new QToolButton(notesToolbar);
    notesNextPageButton->setArrowType(Qt::RightArrow);
    notesNextPageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    notesNextPageButton->setToolTip("Next notes page");
    notesToolbarLayout->addWidget(notesNextPageButton);

    notesAddPageButton = new QToolButton(notesToolbar);
    notesAddPageButton->setIconSize(notesIconSize);
    {
        QFont navFontAdd = notesAddPageButton->font();
        notesAddPageButton->setIcon(makeTextIcon(QStringLiteral("+"), navFontAdd));
    }
    notesAddPageButton->setToolTip("Add notes page");
    notesToolbarLayout->addWidget(notesAddPageButton);

    notesPageJumpEdit = new QLineEdit(notesToolbar);
    notesPageJumpEdit->setFixedWidth(40);
    notesPageJumpEdit->setAlignment(Qt::AlignCenter);
    notesPageJumpEdit->setPlaceholderText("Go");
    notesPageJumpEdit->setToolTip("Type a page number and press Enter to jump");
    notesToolbarLayout->addWidget(notesPageJumpEdit);

    notesPageStatusLabel = new QLabel("", notesToolbar);
    notesPageStatusLabel->setMinimumWidth(80);
    notesPageStatusLabel->setAlignment(Qt::AlignCenter);
    notesToolbarLayout->addWidget(notesPageStatusLabel);

    if (notesPageJumpEdit) {
        connect(notesPageJumpEdit, &QLineEdit::returnPressed, this, [this]() {
            if (!notesPageJumpEdit) {
                return;
            }
            bool ok = false;
            int targetPage = notesPageJumpEdit->text().trimmed().toInt(&ok);
            if (!ok || targetPage <= 0) {
                QToolTip::showText(notesPageJumpEdit->mapToGlobal(QPoint()),
                                   "Enter a valid page number (1 - " + QString::number(notesPages.size()) + ")");
                return;
            }

            int targetIndex = targetPage - 1;
            if (targetIndex < 0 || targetIndex >= notesPages.size()) {
                QToolTip::showText(notesPageJumpEdit->mapToGlobal(QPoint()),
                                   "Page " + QString::number(targetPage) + " does not exist.");
                return;
            }

            if (!notesEditor) {
                return;
            }

            // Save current page content
            if (currentNotesPageIndex >= 0 && currentNotesPageIndex < notesPages.size()) {
                notesPages[currentNotesPageIndex] = notesEditor->toHtml();
            }

            currentNotesPageIndex = targetIndex;
            QString html;
            if (currentNotesPageIndex >= 0 && currentNotesPageIndex < notesPages.size()) {
                html = notesPages[currentNotesPageIndex];
            }

            {
                QSignalBlocker blocker(notesEditor);
                notesEditor->setHtml(html);
            }

            notesLastGoodHtml = notesEditor->toHtml();
            notesPageFull = false;
            onNotesTextChanged();
        });
    }

    notesShowButton->setAutoRaise(true);
    notesBoldButton->setAutoRaise(true);
    notesItalicButton->setAutoRaise(true);
    notesUnderlineButton->setAutoRaise(true);
    notesColorButton->setAutoRaise(true);
    notesHighlightButton->setAutoRaise(true);
    alignLeftButton->setAutoRaise(true);
    alignCenterButton->setAutoRaise(true);
    alignRightButton->setAutoRaise(true);
    bulletListButton->setAutoRaise(true);
    numberListButton->setAutoRaise(true);
    notesPrevPageButton->setAutoRaise(true);
    notesNextPageButton->setAutoRaise(true);
    notesAddPageButton->setAutoRaise(true);

    const QString notesToolbarStyle = QStringLiteral(
                "QToolButton {"
                "  border: none;"
                "  padding: 4px 8px;"
                "  border-radius: 5px;"
                "}"
                "QToolButton:hover {"
                "  background-color: rgba(255, 255, 255, 30);"
                "}"
                "QToolButton#notesColorButton {"
                "  color: rgb(220, 40, 40);"
                "}"
                "QToolButton:checked {"
                "  background-color: rgba(53, 132, 228, 200);"
                "  color: white;"
                "}"
                "QToolButton#notesShowButton {"
                "  font-weight: 600;"
                "  padding: 4px 14px;"
                "  border-radius: 16px;"
                "  background-color: #2563eb;"
                "  color: white;"
                "}"
                "QToolButton#notesShowButton:hover {"
                "  background-color: #1d4ed8;"
                "}"
                "QToolButton#notesShowButton:checked {"
                "  background-color: #16a34a;"
                "  color: white;"
                "}");
    notesToolbar->setStyleSheet(notesToolbarStyle);

    notesLayout->addWidget(notesToolbar);

    // Notes editor (embedded in a graphics view so we can scale the preview
    // visually without altering the actual document formatting or font size
    // used for projection).
    notesEditor = new QTextEdit();
    notesEditor->setAcceptRichText(true);

    // Make the placeholder text (and initial base font) a bit larger so the
    // "Type projection notes here..." hint is easier to read in the preview.
    {
        QFont f = notesEditor->font();
        f.setPointSizeF(f.pointSizeF() * 5);
        notesEditor->setFont(f);
    }
    notesEditor->setPlaceholderText("Type projection notes here...");
    // Remove extra padding so the content area is clean and lines up visually
    notesEditor->setFrameStyle(QFrame::NoFrame);
    notesEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    notesEditor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    notesEditor->setLineWrapMode(QTextEdit::WidgetWidth);
    if (notesEditor->document()) {
        notesEditor->document()->setDocumentMargin(0);
    }

    notesScene = new QGraphicsScene(notesPage);
    notesProxy = notesScene->addWidget(notesEditor);
    notesPreviewView = new QGraphicsView(notesScene, notesPage);
    notesPreviewView->setFrameStyle(QFrame::NoFrame);
    notesPreviewView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    notesPreviewView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    notesPreviewView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    notesPreviewView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    notesLayout->addWidget(notesPreviewView);

    // Initialise notes paging state (start with a single empty page)
    notesPages.clear();
    notesPages.append(QString());
    currentNotesPageIndex = 0;
    notesLastGoodHtml = notesEditor->toHtml();
    notesPageFull = false;
    if (notesPageStatusLabel) {
        notesPageStatusLabel->setText("Page 1");
    }

    notesTabWidget->addTab(notesPage, QStringLiteral("Projection Notes"));

    // --- Tab 2: YouTube ---
    QWidget *youtubePage = new QWidget(notesTabWidget);
    QVBoxLayout *youtubeLayout = new QVBoxLayout(youtubePage);

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    QHBoxLayout *ytControlsLayout = new QHBoxLayout();
    QPushButton *projectVideoButton = new QPushButton(QStringLiteral("Project Video"), youtubePage);
    projectVideoButton->setToolTip("Project the current YouTube video to the projection canvas");
    ytControlsLayout->addWidget(projectVideoButton);

    QPushButton *addToPlaylistButton = new QPushButton(QStringLiteral("Add to Playlist"), youtubePage);
    addToPlaylistButton->setToolTip("Add the current YouTube URL to the playlist");
    ytControlsLayout->addWidget(addToPlaylistButton);

    QToolButton *youtubeAudioToggle = new QToolButton(youtubePage);
    youtubeAudioToggle->setText(QStringLiteral("Unmute"));
    youtubeAudioToggle->setToolTip(QStringLiteral("Toggle audio in the YouTube tab"));
    youtubeAudioToggle->setCheckable(true);
    youtubeAudioToggle->setChecked(false); // start muted
    ytControlsLayout->addWidget(youtubeAudioToggle);

    ytControlsLayout->addStretch();

    youtubeBrowser = new QWebEngineView(youtubePage);
    youtubeBrowser->setUrl(QUrl(QStringLiteral("https://www.youtube.com")));
    if (QWebEnginePage *ytPage = youtubeBrowser->page()) {
        ytPage->setAudioMuted(true);
    }

    // Inject a small CSS snippet on each load in the YouTube tab to hide
    // some of YouTube's visible ad UI (overlay banners, player ad modules,
    // in-feed ad cards, etc.). This does not remove all ads, but it helps
    // declutter the interface for projection use.
    connect(youtubeBrowser, &QWebEngineView::loadFinished,
            this, [this](bool) {
                if (!youtubeBrowser) {
                    return;
                }
                QWebEnginePage *page = youtubeBrowser->page();
                if (!page) {
                    return;
                }

                const QString js = QStringLiteral(R"JS(
                    (function() {
                        try {
                            if (!location.hostname || location.hostname.indexOf('youtube.com') === -1) {
                                return;
                            }

                            // Hide a number of known YouTube ad UI containers
                            var css = `
ytd-display-ad-renderer,
ytd-video-masthead-ad-v3-renderer,
ytd-companion-slot-renderer,
ytd-action-companion-ad-renderer,
ytd-promoted-video-renderer,
.ytd-player-legacy-desktop-watch-ads-renderer,
.ytp-ad-module,
.ytp-ad-overlay-container,
.ytp-ad-player-overlay,
.ytp-ad-image-overlay,
.ytp-ad-text,
.ytp-ad-preview-container,
#player-ads {
  display: none !important;
}`;
                            var s = document.createElement('style');
                            s.type = 'text/css';
                            s.appendChild(document.createTextNode(css));
                            if (document.head) {
                                document.head.appendChild(s);
                            }

                            // Best-effort auto-skip for skippable ads and closing
                            // overlay ads. This does not affect the actual video
                            // stream URLs, it only clicks visible controls.
                            function spAutoSkipAds() {
                                try {
                                    var skip = document.querySelector('.ytp-ad-skip-button, .ytp-ad-skip-button-modern');
                                    if (skip) {
                                        skip.click();
                                    }
                                    var closeOverlay = document.querySelector('.ytp-ad-overlay-close-button');
                                    if (closeOverlay) {
                                        closeOverlay.click();
                                    }
                                } catch (e) {
                                    // ignore
                                }
                            }

                            setInterval(spAutoSkipAds, 1000);
                        } catch (e) {
                            console.error('SimplePresenter YouTube ad CSS/skip inject failed', e);
                        }
                    })();
                )JS");

                page->runJavaScript(js);
            });

    youtubeLayout->addLayout(ytControlsLayout);
    youtubeLayout->addWidget(youtubeBrowser);

    connect(youtubeAudioToggle, &QToolButton::toggled, this, [this, youtubeAudioToggle](bool enabled) {
        if (!youtubeBrowser) {
            return;
        }
        if (QWebEnginePage *page = youtubeBrowser->page()) {
            page->setAudioMuted(!enabled);
        }
        if (youtubeAudioToggle) {
            youtubeAudioToggle->setText(enabled ? QStringLiteral("Mute")
                                               : QStringLiteral("Unmute"));
        }
    });

    connect(projectVideoButton, &QPushButton::clicked, this, [this, youtubeAudioToggle]() {
        if (!youtubeBrowser) {
            return;
        }

        // Pause playback in the YouTube tab so only the projected player is active
        if (QWebEnginePage *page = youtubeBrowser->page()) {
            page->runJavaScript(QStringLiteral(
                "try {"
                "  var vids = document.querySelectorAll('video');"
                "  vids.forEach(function(v){ v.pause(); });"
                "} catch (e) {}"));
            page->setAudioMuted(true);
        }
        if (youtubeAudioToggle) {
            youtubeAudioToggle->setChecked(false);
        }

        const QString url = youtubeBrowser->url().toString();
        if (projectionCanvas) {
            projectionCanvas->showYouTubeVideo(url);
        }
        if (fullscreenProjection) {
            fullscreenProjection->showYouTubeVideo(url);
            // External monitor projection should remain muted; audio is
            // controlled only on the preview canvas.
            fullscreenProjection->setYouTubeMuted(true);
        }

        // Update OBS overlay to show this YouTube video instead of local media
        if (overlayServer) {
            overlayServer->updateOverlay("", "");
            overlayServer->updateMedia("", false);
            overlayServer->updateYouTube(url);
            // Force the OBS browser source to reload so the new YouTube
            // video always appears without needing a manual refresh.
            overlayServer->triggerRefresh();
        }

        // Enable media controls for YouTube playback so the user can
        // drive play/pause and scrubbing from the app. Assume autoplay
        // starts the YouTube video, so reflect an initial "playing" state.
        youtubePlaying = true;
        onMediaVideoStarted();

        // After the OBS browser source reloads and the overlay's YouTube
        // player has had time to initialise, resync its playback position
        // to match the Projection Canvas so they stay in sync even if the
        // overlay loaded a bit later.
        QTimer::singleShot(1500, this, [this]() { syncYouTubeOverlayToProjection(); });
        QTimer::singleShot(4000, this, [this]() { syncYouTubeOverlayToProjection(); });
    });

    connect(addToPlaylistButton, &QPushButton::clicked, this, [this]() {
        if (!youtubeBrowser || !playlistPanel) {
            return;
        }
        const QUrl url = youtubeBrowser->url();
        if (!url.isValid() || url.isEmpty()) {
            return;
        }
        const QString urlString = url.toString();
        QString title;
        if (QWebEnginePage *page = youtubeBrowser->page()) {
            // Pause any playing video in the YouTube tab when adding it to the
            // playlist so audio does not continue in the background.
            page->runJavaScript(QStringLiteral(
                "try {"
                "  var vids = document.querySelectorAll('video');"
                "  vids.forEach(function(v){ v.pause(); });"
                "} catch (e) {}"));
            title = page->title();
        }
        playlistPanel->addYouTube(urlString, title);
    });

    notesTabWidget->addTab(youtubePage, QStringLiteral("YouTube"));
#else
    QLabel *label = new QLabel(
                "YouTube support requires the Qt WebEngine component.\n"
                "Install Qt WebEngine via the Qt Maintenance Tool and rebuild.",
                youtubePage);
    label->setAlignment(Qt::AlignCenter);
    youtubeLayout->addWidget(label);
    notesTabWidget->addTab(youtubePage, QStringLiteral("YouTube (Unavailable)"));
#endif

    previewLayout->addWidget(notesTabWidget, 1);
    
    verticalSplitter->addWidget(previewWidget);
    
    // Set splitter proportions (60% controls, 40% previews)
    verticalSplitter->setStretchFactor(0, 3);
    verticalSplitter->setStretchFactor(1, 2);
    
    setCentralWidget(verticalSplitter);

    // Helper to merge character formatting into current selection in notes editor
    auto mergeNotesCharFormat = [this](const QTextCharFormat &format) {
        if (!notesEditor) {
            return;
        }
        QTextCursor cursor = notesEditor->textCursor();
        cursor.mergeCharFormat(format);
        notesEditor->mergeCurrentCharFormat(format);
    };

    // Connect notes toolbar actions
    connect(notesFontCombo, &QFontComboBox::currentFontChanged,
            this, [mergeNotesCharFormat](const QFont &font) {
                QTextCharFormat fmt;
                fmt.setFontFamilies(QStringList() << font.family());
                mergeNotesCharFormat(fmt);
            });

    connect(notesSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [notesSizeCombo, mergeNotesCharFormat](int index) {
                bool ok = false;
                int size = notesSizeCombo->itemData(index).toInt(&ok);
                if (!ok) {
                    size = notesSizeCombo->itemText(index).toInt(&ok);
                }
                if (!ok || size <= 0) {
                    return;
                }
                QTextCharFormat fmt;
                fmt.setFontPointSize(size);
                mergeNotesCharFormat(fmt);
            });

    connect(notesBoldButton, &QToolButton::toggled,
            this, [mergeNotesCharFormat](bool checked) {
                QTextCharFormat fmt;
                fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
                mergeNotesCharFormat(fmt);
            });

    connect(notesItalicButton, &QToolButton::toggled,
            this, [mergeNotesCharFormat](bool checked) {
                QTextCharFormat fmt;
                fmt.setFontItalic(checked);
                mergeNotesCharFormat(fmt);
            });

    connect(notesUnderlineButton, &QToolButton::toggled,
            this, [mergeNotesCharFormat](bool checked) {
                QTextCharFormat fmt;
                fmt.setFontUnderline(checked);
                mergeNotesCharFormat(fmt);
            });

    connect(notesColorButton, &QToolButton::clicked,
            this, [this, mergeNotesCharFormat]() {
                QColor color = QColorDialog::getColor(Qt::white, this, "Choose Notes Text Color");
                if (!color.isValid()) {
                    return;
                }
                QTextCharFormat fmt;
                fmt.setForeground(color);
                mergeNotesCharFormat(fmt);
            });

    connect(chooseHighlightAction, &QAction::triggered,
            this, [this, mergeNotesCharFormat]() {
                QColor color = QColorDialog::getColor(Qt::yellow, this, "Choose Notes Highlight Color",
                                                     QColorDialog::ShowAlphaChannel);
                if (!color.isValid()) {
                    return;
                }
                QTextCharFormat fmt;
                fmt.setBackground(color);
                mergeNotesCharFormat(fmt);
            });

    connect(clearHighlightAction, &QAction::triggered,
            this, [mergeNotesCharFormat]() {
                QTextCharFormat fmt;
                fmt.setBackground(Qt::transparent);
                mergeNotesCharFormat(fmt);
            });

    connect(alignLeftButton, &QToolButton::clicked,
            this, [this]() {
                if (notesEditor) {
                    notesEditor->setAlignment(Qt::AlignLeft);
                }
            });
    connect(alignCenterButton, &QToolButton::clicked,
            this, [this]() {
                if (notesEditor) {
                    notesEditor->setAlignment(Qt::AlignHCenter);
                }
            });
    connect(alignRightButton, &QToolButton::clicked,
            this, [this]() {
                if (notesEditor) {
                    notesEditor->setAlignment(Qt::AlignRight);
                }
            });

    connect(bulletListButton, &QToolButton::clicked,
            this, [this]() {
                if (!notesEditor) {
                    return;
                }
                QTextCursor cursor = notesEditor->textCursor();
                QTextListFormat listFormat;
                listFormat.setStyle(QTextListFormat::ListDisc);
                cursor.beginEditBlock();
                cursor.createList(listFormat);
                cursor.endEditBlock();
            });

    connect(numberListButton, &QToolButton::clicked,
            this, [this]() {
                if (!notesEditor) {
                    return;
                }
                QTextCursor cursor = notesEditor->textCursor();
                QTextListFormat listFormat;
                listFormat.setStyle(QTextListFormat::ListDecimal);
                cursor.beginEditBlock();
                cursor.createList(listFormat);
                cursor.endEditBlock();
            });

    // Notes editor content -> paging logic and projection canvases
    connect(notesEditor, &QTextEdit::textChanged,
            this, &MainWindow::onNotesTextChanged);

    // Notes paging button actions
    connect(notesPrevPageButton, &QToolButton::clicked,
            this, &MainWindow::onNotesPrevPage);
    connect(notesNextPageButton, &QToolButton::clicked,
            this, &MainWindow::onNotesNextPage);
    connect(notesAddPageButton, &QToolButton::clicked,
            this, &MainWindow::onNotesAddPage);

    // Show/hide notes overlay on projection (and push to OBS overlay)
    connect(notesShowButton, &QToolButton::toggled,
            this, [this](bool checked) {
                notesShowButton->setText(checked ? "Hide" : "Show");
                const QString html = notesEditor ? notesEditor->toHtml() : QString();
                if (projectionCanvas) {
                    projectionCanvas->setNotesHtml(html);
                    projectionCanvas->setNotesVisible(checked);
                    projectionCanvas->setNotesModeActive(checked);
                }
                if (fullscreenProjection) {
                    fullscreenProjection->setNotesHtml(html);
                    fullscreenProjection->setNotesVisible(checked);
                    fullscreenProjection->setNotesModeActive(checked);
                }

                if (overlayServer) {
                    // For OBS, mirror the Projection Canvas notes as a
                    // transparent PNG so layout matches exactly.
                    if (projectionCanvas && checked) {
                        QImage notesImage = projectionCanvas->getNotesImage();
                        overlayServer->updateNotesImage(notesImage, true);
                    } else {
                        overlayServer->updateNotesImage(QImage(), false);
                    }

                    if (checked) {
                        // When notes are shown, use the Notes Background settings
                        // as the OBS overlay media background.
                        QSettings bgSettings("SimplePresenter", "SimplePresenter");
                        bgSettings.beginGroup("Backgrounds");
                        const QString type = bgSettings.value("notes/backgroundType", QStringLiteral("color")).toString();
                        const QString imagePath = bgSettings.value("notes/image").toString();
                        const QString videoPath = bgSettings.value("notes/video").toString();
                        bgSettings.endGroup();

                        QString mediaPath;
                        bool isVideo = false;
                        if (type == QLatin1String("video") && !videoPath.isEmpty()) {
                            mediaPath = videoPath;
                            isVideo = true;
                        } else if (type == QLatin1String("image") && !imagePath.isEmpty()) {
                            mediaPath = imagePath;
                            isVideo = false;
                        }

                        overlayServer->updateOverlay("", "");
                        overlayServer->updateYouTube("");
                        overlayServer->updateMedia(mediaPath, isVideo);
                        overlayServer->triggerRefresh();
                    } else {
                        // Hide notes overlay in OBS; next Bible/lyrics/media projection
                        // will repopulate overlay content.
                        overlayServer->updateNotesImage(QImage(), false);
                        overlayServer->updateMedia("", false);
                        overlayServer->updateOverlay("", "");
                        overlayServer->updateYouTube("");
                    }
                }
            });
    
    // Connect signals
    connect(biblePanel, &BiblePanel::verseSelected,
            this, &MainWindow::projectBibleVerse);
    connect(songPanel, &SongPanel::sectionSelected,
            this, &MainWindow::projectSongSection);
    connect(playlistPanel, &PlaylistPanel::itemActivated,
            this, &MainWindow::projectPlaylistItem);
    connect(playlistPanel, &PlaylistPanel::bibleVerseActivated,
            [this](const QString &reference) {
                contentTabs->setCurrentWidget(biblePanel);  // Switch to Bible tab
                biblePanel->activateVerse(reference);
            });
    connect(playlistPanel, &PlaylistPanel::songActivated,
            [this](const QString &songTitle) {
                contentTabs->setCurrentWidget(songPanel);  // Switch to Songs tab
                songPanel->activateSong(songTitle);
            });
    connect(playlistPanel, &PlaylistPanel::youtubeActivated,
            [this](const QString &url) {
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
                // When a YouTube item is triggered from the playlist, project
                // it to the canvases and OBS without changing the YouTube tab
                // contents in the UI.
                if (!url.isEmpty()) {
                    if (projectionCanvas) {
                        projectionCanvas->showYouTubeVideo(url);
                    }
                    if (fullscreenProjection) {
                        fullscreenProjection->showYouTubeVideo(url);
                        fullscreenProjection->setYouTubeMuted(true);
                    }
                    if (overlayServer) {
                        overlayServer->updateOverlay("", "");
                        overlayServer->updateMedia("", false);
                        overlayServer->updateYouTube(url);
                        overlayServer->triggerRefresh();
                    }
                    youtubePlaying = true;
                    onMediaVideoStarted();
                    QTimer::singleShot(1500, this, [this]() { syncYouTubeOverlayToProjection(); });
                    QTimer::singleShot(4000, this, [this]() { syncYouTubeOverlayToProjection(); });
                }
#else
                Q_UNUSED(url);
#endif
            });
    
    // Connect add to playlist signals
    connect(biblePanel, &BiblePanel::addVerseToPlaylist,
            playlistPanel, &PlaylistPanel::addBibleVerse);
    connect(songPanel, &SongPanel::addSongToPlaylist,
            [this](const QString &songTitle) { playlistPanel->addSong(songTitle); });
    connect(songPanel, &SongPanel::addSectionToPlaylist,
            [this](const QString &songTitle, const QString &sectionText) {
                playlistPanel->addSong(songTitle, sectionText);
            });
    connect(mediaPanel, &MediaPanel::mediaAddedToPlaylist,
            playlistPanel, &PlaylistPanel::addMedia);
    connect(mediaPanel, &MediaPanel::mediaSelected,
            this, &MainWindow::projectMediaItem);
    connect(mediaPanel, &MediaPanel::mediaAboutToRemove,
            this, &MainWindow::onMediaEntryAboutToRemove);
    connect(mediaPanel, &MediaPanel::mediaRemoved,
            this, &MainWindow::onMediaEntryRemoved);
    connect(playlistPanel, &PlaylistPanel::mediaActivated,
            this, &MainWindow::projectMediaItem);
    if (powerPointPanel) {
        connect(powerPointPanel, &PowerPointPanel::slideImageAvailable,
                this, [this](const QString &imagePath, bool useFade) {
                    if (imagePath.isEmpty()) {
                        return;
                    }
                    if (projectionCanvas) {
                        projectionCanvas->setNextImageFade(useFade);
                    }
                    if (fullscreenProjection) {
                        fullscreenProjection->setNextImageFade(useFade);
                    }
                    // Treat exported slide as a still image on the projection canvas
                    projectMediaItem(imagePath, /*isVideo=*/false);
                });
    }
    connect(projectionCanvas, &ProjectionCanvas::mediaVideoPlaybackStarted,
            this, &MainWindow::onMediaVideoStarted);
    connect(projectionCanvas, &ProjectionCanvas::mediaVideoPlaybackStopped,
            this, &MainWindow::onMediaVideoStopped);
    connect(contentTabs, &QTabWidget::currentChanged,
            mediaPanel, &MediaPanel::onTabChanged);

    if (QMediaPlayer *player = projectionCanvas->mediaPlayer()) {
        connect(player, &QMediaPlayer::playbackStateChanged,
                this, &MainWindow::onMediaPlaybackStateChanged);
        connect(player, &QMediaPlayer::positionChanged,
                this, &MainWindow::onMediaPositionChanged);
        connect(player, &QMediaPlayer::durationChanged,
                this, &MainWindow::onMediaDurationChanged);
    }
}

void MainWindow::setupMenuBar()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    newAction = fileMenu->addAction("&New Playlist");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newPlaylist);
    
    openAction = fileMenu->addAction("&Open Playlist...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openPlaylist);
    
    saveAction = fileMenu->addAction("&Save Playlist");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::savePlaylist);
    
    QAction *saveAsAction = fileMenu->addAction("Save Playlist &As...");
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::savePlaylistAs);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // Tools menu
    QMenu *toolsMenu = menuBar()->addMenu("&Tools");
    clearAction = toolsMenu->addAction("Clear Overlays");
    clearAction->setShortcut(Qt::Key_Escape);
    connect(clearAction, &QAction::triggered, this, &MainWindow::clearOverlays);
    toolsMenu->addSeparator();
    settingsAction = toolsMenu->addAction("&Settings...");
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *checkUpdatesAction = helpMenu->addAction("Check for &Updates...");
    connect(checkUpdatesAction, &QAction::triggered, this, &MainWindow::onCheckForUpdates);
    helpMenu->addSeparator();

    QAction *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    
    toolbar->addAction(newAction);
    toolbar->addAction(openAction);
    toolbar->addAction(saveAction);
    toolbar->addSeparator();
    toolbar->addAction(clearAction);
    toolbar->addSeparator();
    toolbar->addAction(settingsAction);
    toolbar->addSeparator();
    toggleProjectionDisplayAction = toolbar->addAction("Projection Display");
    toggleProjectionDisplayAction->setCheckable(true);
    connect(toggleProjectionDisplayAction, &QAction::toggled,
            this, &MainWindow::toggleExternalProjection);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("Ready");
}

void MainWindow::setupMediaControls(QVBoxLayout *projectionLayout)
{
    mediaControlsWidget = new QWidget(projectionLayout->parentWidget());
    mediaControlsWidget->setVisible(false);
    mediaControlsWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QHBoxLayout *controlsLayout = new QHBoxLayout(mediaControlsWidget);
    controlsLayout->setContentsMargins(4, 0, 4, 0);
    controlsLayout->setSpacing(8);

    mediaPlayPauseButton = new QToolButton(mediaControlsWidget);
    mediaPlayPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    mediaPlayPauseButton->setToolTip("Play/Pause");
    controlsLayout->addWidget(mediaPlayPauseButton, 0);

    mediaSeekSlider = new QSlider(Qt::Horizontal, mediaControlsWidget);
    mediaSeekSlider->setRange(0, 0);
    mediaSeekSlider->setEnabled(false);
    mediaSeekSlider->setTracking(true);
    controlsLayout->addWidget(mediaSeekSlider, 1);

    mediaVolumeSlider = new QSlider(Qt::Horizontal, mediaControlsWidget);
    mediaVolumeSlider->setRange(0, 100);
    mediaVolumeSlider->setValue(50);
    mediaVolumeSlider->setFixedWidth(120);
    mediaVolumeSlider->setToolTip("Volume");
    controlsLayout->addWidget(mediaVolumeSlider, 0);

    projectionLayout->addWidget(mediaControlsWidget);

    connect(mediaPlayPauseButton, &QToolButton::clicked,
            this, &MainWindow::onMediaPlayPauseClicked);
    connect(mediaSeekSlider, &QSlider::sliderPressed,
            this, &MainWindow::onMediaSeekSliderPressed);
    connect(mediaSeekSlider, &QSlider::sliderReleased,
            this, &MainWindow::onMediaSeekSliderReleased);
    connect(mediaSeekSlider, &QSlider::sliderMoved,
            this, &MainWindow::onMediaSeekSliderMoved);
    connect(mediaVolumeSlider, &QSlider::valueChanged,
            this, &MainWindow::onMediaVolumeChanged);

    if (projectionCanvas && projectionCanvas->mediaAudioOutput()) {
        QAudioOutput *output = projectionCanvas->mediaAudioOutput();
        mediaVolumeSlider->setValue(qBound(0, static_cast<int>(output->volume() * 100.0 + 0.5), 100));
    }
}

void MainWindow::updateMediaPlayPauseIcon()
{
    if (!mediaPlayPauseButton) {
        return;
    }

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    // When a YouTube video is active, reflect the internal YouTube
    // playback state instead of QMediaPlayer.
    if (projectionCanvas && projectionCanvas->isYouTubeActive()) {
        mediaPlayPauseButton->setIcon(style()->standardIcon(
            youtubePlaying ? QStyle::SP_MediaPause
                           : QStyle::SP_MediaPlay));
        return;
    }
#endif

    QMediaPlayer *player = projectionCanvas ? projectionCanvas->mediaPlayer() : nullptr;
    const bool playing = player && player->playbackState() == QMediaPlayer::PlayingState;
    mediaPlayPauseButton->setIcon(style()->standardIcon(playing ? QStyle::SP_MediaPause
                                                                : QStyle::SP_MediaPlay));
}

void MainWindow::loadSettings()
{
    // Restore window geometry
    restoreGeometry(settings->value("geometry").toByteArray());
    restoreState(settings->value("windowState").toByteArray());
    
    // Restore splitter states
    if (settings->contains("mainSplitterState")) {
        mainSplitter->restoreState(settings->value("mainSplitterState").toByteArray());
    }
    if (settings->contains("verticalSplitterState")) {
        verticalSplitter->restoreState(settings->value("verticalSplitterState").toByteArray());
    }
    
    // Load canvas settings
    projectionCanvas->loadSettings(false);
    
    // Don't auto-load last playlist - start with empty playlist
    // User can manually open a playlist if needed
}

void MainWindow::saveSettings()
{
    settings->setValue("geometry", saveGeometry());
    settings->setValue("windowState", saveState());
    settings->setValue("mainSplitterState", mainSplitter->saveState());
    settings->setValue("verticalSplitterState", verticalSplitter->saveState());
    settings->setValue("lastPlaylist", currentPlaylistFile);
}

void MainWindow::loadServiceNotes(const QString &filePath)
{
    if (filePath.isEmpty() || !notesEditor) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject notesObj = root.value("notes").toObject();

    QJsonArray pagesArray = notesObj.value("pages").toArray();
    int loadedCurrentIndex = notesObj.value("currentPage").toInt(0);

    notesPages.clear();

    if (pagesArray.isEmpty()) {
        // No notes persisted: start with a single empty page
        notesPages.append(QString());
        currentNotesPageIndex = 0;
        {
            QSignalBlocker blocker(notesEditor);
            notesEditor->clear();
        }
        notesLastGoodHtml = notesEditor->toHtml();
        notesPageFull = false;
        if (notesPageStatusLabel) {
            notesPageStatusLabel->setStyleSheet(QString());
            notesPageStatusLabel->setText("Page 1");
        }
        onNotesTextChanged();
        return;
    }

    for (const QJsonValue &val : pagesArray) {
        notesPages.append(val.toString());
    }

    if (notesPages.isEmpty()) {
        notesPages.append(QString());
    }

    currentNotesPageIndex = qBound(0, loadedCurrentIndex, notesPages.size() - 1);

    // Load the current page HTML into the editor without triggering recursive updates
    QString html = notesPages.value(currentNotesPageIndex);
    {
        QSignalBlocker blocker(notesEditor);
        notesEditor->setHtml(html);
    }

    notesLastGoodHtml = notesEditor->toHtml();
    notesPageFull = false;
    if (notesPageStatusLabel) {
        notesPageStatusLabel->setStyleSheet(QString());
        notesPageStatusLabel->setText(QString("Page %1").arg(currentNotesPageIndex + 1));
    }

    // Re-evaluate fullness and push HTML to projection
    onNotesTextChanged();
}

void MainWindow::saveServiceNotes(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    // Ensure the latest edits are captured in the current page
    if (notesEditor && currentNotesPageIndex >= 0 && currentNotesPageIndex < notesPages.size()) {
        notesPages[currentNotesPageIndex] = notesEditor->toHtml();
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();

    // Build notes JSON structure
    QJsonObject notesObj;
    QJsonArray pagesArray;
    if (!notesPages.isEmpty()) {
        for (const QString &html : std::as_const(notesPages)) {
            pagesArray.append(html);
        }
    } else if (notesEditor) {
        // Fallback: at least persist the current editor content as one page
        pagesArray.append(notesEditor->toHtml());
    }

    notesObj["pages"] = pagesArray;
    notesObj["currentPage"] = currentNotesPageIndex;

    root["notes"] = notesObj;

    doc.setObject(root);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}


void MainWindow::newPlaylist()
{
    if (playlistPanel->hasUnsavedChanges()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Save Changes?",
            "Do you want to save changes to the current playlist?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
        );
        
        if (reply == QMessageBox::Yes) {
            savePlaylist();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }
    
    playlistPanel->clear();
    currentPlaylistFile.clear();
    setWindowTitle("SimplePresenter - New Playlist");
}

void MainWindow::openPlaylist()
{
#ifdef Q_OS_MACOS
    QFileDialog dialog(this, "Open Playlist");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory("data/services");
    dialog.setNameFilter("Simple Presenter Playlists (*.spp);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QString fileName = dialog.selectedFiles().value(0);
    if (fileName.isEmpty()) {
        return;
    }
#else
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Playlist",
        "data/services",
        "Simple Presenter Playlists (*.spp);;All Files (*)");

    if (fileName.isEmpty()) {
        return;
    }
#endif
    if (playlistPanel->loadPlaylist(fileName)) {
        currentPlaylistFile = fileName;
        setWindowTitle(QString("SimplePresenter - %1").arg(QFileInfo(fileName).fileName()));
        statusBar()->showMessage("Playlist loaded", 3000);
        // Load any notes that were saved with this service
        loadServiceNotes(fileName);
    } else {
        QMessageBox::warning(this, "Error", "Failed to load playlist");
    }
}

void MainWindow::savePlaylist()
{
    if (currentPlaylistFile.isEmpty()) {
        savePlaylistAs();
    } else {
        if (playlistPanel->savePlaylist(currentPlaylistFile)) {
            // Persist notes together with the playlist JSON
            saveServiceNotes(currentPlaylistFile);
            statusBar()->showMessage("Playlist saved", 3000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save playlist");
        }
    }
}

void MainWindow::savePlaylistAs()
{
#ifdef Q_OS_MACOS
    QFileDialog dialog(this, "Save Playlist As");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory("data/services");
    dialog.setNameFilter("Simple Presenter Playlists (*.spp);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QString fileName = dialog.selectedFiles().value(0);
    if (fileName.isEmpty()) {
        return;
    }
#else
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Playlist As",
        "data/services",
        "Simple Presenter Playlists (*.spp);;All Files (*)");

    if (fileName.isEmpty()) {
        return;
    }
#endif
    if (!fileName.endsWith(".spp", Qt::CaseInsensitive)) {
        fileName += ".spp";
    }

    if (playlistPanel->savePlaylist(fileName)) {
        currentPlaylistFile = fileName;
        setWindowTitle(QString("SimplePresenter - %1").arg(QFileInfo(fileName).fileName()));
        // Persist notes together with the playlist JSON
        saveServiceNotes(fileName);
        statusBar()->showMessage("Playlist saved", 3000);
    } else {
        QMessageBox::warning(this, "Error", "Failed to save playlist");
    }
}

void MainWindow::showSettings()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        projectionCanvas->loadSettings();
        biblePanel->refreshAvailableBibles();
        int displayIndex = 0;
        {
            QSettings settings("SimplePresenter", "SimplePresenter");
            settings.beginGroup("ProjectionCanvas");
            displayIndex = settings.value("displayIndex", 0).toInt();
            settings.endGroup();
        }
        QList<QScreen*> screens = QGuiApplication::screens();
        if (displayIndex >= 0 && displayIndex < screens.size()) {
            QScreen *targetScreen = screens[displayIndex];
            
            // Create or update fullscreen projection window
            if (!fullscreenProjection) {
                fullscreenProjection = new ProjectionCanvas(nullptr);
                fullscreenProjection->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
            }
            
            // Move window to target screen and show fullscreen
            fullscreenProjection->setGeometry(targetScreen->geometry());
            fullscreenProjection->show();  // Create window handle
            
            if (fullscreenProjection->windowHandle()) {
                fullscreenProjection->windowHandle()->setScreen(targetScreen);
            }
            
            fullscreenProjection->loadSettings();
            fullscreenProjection->showFullScreen();
        }
        
        // Reload overlay server settings
        if (overlayServer) {
            overlayServer->loadSettings();
            overlayServer->triggerRefresh();  // Trigger OBS browser refresh
        }

        // Force repaint to show new settings
        projectionCanvas->repaint();

        statusBar()->showMessage("Settings saved and applied", 3000);
    }
}

void MainWindow::toggleExternalProjection(bool checked)
{
    if (checked) {
        int displayIndex = 0;
        {
            QSettings settings("SimplePresenter", "SimplePresenter");
            settings.beginGroup("ProjectionCanvas");
            displayIndex = settings.value("displayIndex", 0).toInt();
            settings.endGroup();
        }
        QList<QScreen*> screens = QGuiApplication::screens();
        if (displayIndex >= 0 && displayIndex < screens.size()) {
            QScreen *targetScreen = screens[displayIndex];
            if (!fullscreenProjection) {
                fullscreenProjection = new ProjectionCanvas(nullptr);
                fullscreenProjection->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
            }
            fullscreenProjection->setGeometry(targetScreen->geometry());
            fullscreenProjection->show();
            if (fullscreenProjection->windowHandle()) {
                fullscreenProjection->windowHandle()->setScreen(targetScreen);
            }
            fullscreenProjection->loadSettings();
            fullscreenProjection->showFullScreen();
        } else {
            if (toggleProjectionDisplayAction) {
                toggleProjectionDisplayAction->setChecked(false);
            }
        }
    } else {
        if (fullscreenProjection) {
            fullscreenProjection->close();
            fullscreenProjection->deleteLater();
            fullscreenProjection = nullptr;
        }
    }
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About SimplePresenter",
        "<h2>SimplePresenter 1.0</h2>"
        "<p>A lightweight Bible & Lyrics projection tool for churches.</p>"
        "<p>Features:</p>"
        "<ul>"
        "<li>Bible verse projection with quick search</li>"
        "<li>Song lyrics with section-based display</li>"
        "<li>Service playlist management</li>"
        "<li>Projection preview canvas</li>"
        "</ul>"
        "<p>Built with Qt 6</p>"
    );
}

void MainWindow::clearOverlays()
{
    if (projectionCanvas) {
        projectionCanvas->clearOverlay();
    }
    
    if (fullscreenProjection) {
        fullscreenProjection->clearOverlay();
    }
    
    if (notesShowButton) {
        notesShowButton->setChecked(false);
    }
    
    // Clear overlay server for OBS
    if (overlayServer) {
        overlayServer->clearOverlay();
    }
    
    statusBar()->showMessage("Overlays cleared", 2000);
}

void MainWindow::projectBibleVerse(const QString &reference, const QString &text)
{
    // When projecting a Bible verse, hide notes overlay so text is visible
    if (notesShowButton && notesShowButton->isChecked()) {
        notesShowButton->setChecked(false);  // toggled handler will clear notes on all canvases
    }

    // Determine whether to display Bible version beside reference
    QString versionName;
    bool displayVersionProjection = false;
    bool displayVersionObs = false;

    if (settings) {
        settings->beginGroup("ProjectionCanvas");
        displayVersionProjection = settings->value("displayVersion", false).toBool();
        settings->endGroup();

        settings->beginGroup("OBSOverlay");
        displayVersionObs = settings->value("refDisplayVersion", false).toBool();
        settings->endGroup();
    }

    if (displayVersionProjection || displayVersionObs) {
        if (biblePanel) {
            versionName = biblePanel->currentTranslationName();
        }
    }

    QString projReference = reference;
    QString obsReference = reference;
    if (!versionName.isEmpty()) {
        if (displayVersionProjection) {
            projReference = QString("%1 (%2)").arg(reference, versionName);
        }
        if (displayVersionObs) {
            obsReference = QString("%1 (%2)").arg(reference, versionName);
        }
    }

    if (projectionCanvas) {
        projectionCanvas->showBibleVerse(projReference, text);
    }
    
    if (fullscreenProjection) {
        fullscreenProjection->showBibleVerse(projReference, text);
    }
    
    // Update overlay server for OBS
    if (overlayServer) {
        overlayServer->updateOverlay(obsReference, text);
        // Clear media and YouTube when displaying Bible verse
        overlayServer->updateMedia("", false);
        overlayServer->updateYouTube("");
    }
    
    statusBar()->showMessage(QString("Projecting: %1").arg(projReference), 3000);
}

void MainWindow::projectSongSection(const QString &songTitle, const QString &sectionText)
{
    // When projecting song lyrics, hide notes overlay so lyrics are visible
    if (notesShowButton && notesShowButton->isChecked()) {
        notesShowButton->setChecked(false);
    }

    if (projectionCanvas) {
        projectionCanvas->showLyrics(songTitle, sectionText);
    }
    
    if (fullscreenProjection) {
        fullscreenProjection->showLyrics(songTitle, sectionText);
    }
    
    // Update overlay server for OBS (lyrics only, no title)
    if (overlayServer) {
        overlayServer->updateOverlay("", sectionText);
        // Clear media and YouTube when displaying lyrics
        overlayServer->updateMedia("", false);
        overlayServer->updateYouTube("");
    }
    
    statusBar()->showMessage(QString("Projecting: %1").arg(songTitle), 3000);
}

void MainWindow::projectPlaylistItem(int index)
{
    // Playlist panel will emit appropriate signals through bible/song panels
    statusBar()->showMessage(QString("Projecting playlist item %1").arg(index + 1), 3000);
}

void MainWindow::onNotesTextChanged()
{
    if (!notesEditor) {
        return;
    }

    QTextDocument *doc = notesEditor->document();
    if (!doc) {
        return;
    }

    const qreal docHeight = doc->size().height();
    const int viewportHeight = notesEditor->viewport() ? notesEditor->viewport()->height() : notesEditor->height();
    if (viewportHeight <= 0) {
        return;
    }

    double ratio = static_cast<double>(docHeight) / static_cast<double>(viewportHeight);
    QString html = notesEditor->toHtml();

    QString labelText = QString("Page %1").arg(currentNotesPageIndex + 1);
    QString labelStyle;

    // Allow content while it fits the current page; warn when almost full;
    // once it overflows, revert to last good HTML and mark page as FULL.
    if (ratio <= 0.9) {
        notesLastGoodHtml = html;
        notesPageFull = false;
        // Normal state
    } else if (ratio <= 1.0) {
        notesLastGoodHtml = html;
        notesPageFull = false;
        labelText = QString("Page %1 (Almost full)").arg(currentNotesPageIndex + 1);
        labelStyle = QStringLiteral("color: #b36b00; font-weight: bold;");
    } else {
        // Overflow: block additional content by reverting to last good HTML
        notesPageFull = true;
        if (!notesLastGoodHtml.isEmpty()) {
            QSignalBlocker blocker(notesEditor);
            notesEditor->setHtml(notesLastGoodHtml);
        }
        html = notesLastGoodHtml;
        labelText = QString("Page %1 FULL").arg(currentNotesPageIndex + 1);
        labelStyle = QStringLiteral("color: red; font-weight: bold;");

        // Recompute document metrics after revert (not strictly required but safe)
        doc = notesEditor->document();
        if (doc) {
            doc->setModified(false);
        }
    }

    // Save current page HTML
    if (currentNotesPageIndex >= 0 && currentNotesPageIndex < notesPages.size()) {
        notesPages[currentNotesPageIndex] = html;
    }

    // Update page status label
    if (notesPageStatusLabel) {
        notesPageStatusLabel->setText(labelText);
        notesPageStatusLabel->setStyleSheet(labelStyle);
    }

    // Push current page HTML to projection canvases
    if (projectionCanvas) {
        projectionCanvas->setNotesHtml(html);
    }
    if (fullscreenProjection) {
        fullscreenProjection->setNotesHtml(html);
    }

    // If notes are currently shown, also mirror the updated notes into the OBS
    // overlay by regenerating the notes image.
    if (overlayServer) {
        bool visible = notesShowButton && notesShowButton->isChecked();
        if (projectionCanvas && visible) {
            QImage notesImage = projectionCanvas->getNotesImage();
            overlayServer->updateNotesImage(notesImage, true);
        } else {
            overlayServer->updateNotesImage(QImage(), false);
        }
    }
}

void MainWindow::onNotesPrevPage()
{
    if (!notesEditor) {
        return;
    }
    if (currentNotesPageIndex <= 0) {
        return;
    }

    // Save current page content
    if (currentNotesPageIndex >= 0 && currentNotesPageIndex < notesPages.size()) {
        notesPages[currentNotesPageIndex] = notesEditor->toHtml();
    }

    // Move to previous page
    --currentNotesPageIndex;
    QString html;
    if (currentNotesPageIndex >= 0 && currentNotesPageIndex < notesPages.size()) {
        html = notesPages[currentNotesPageIndex];
    }

    {
        QSignalBlocker blocker(notesEditor);
        notesEditor->setHtml(html);
    }

    notesLastGoodHtml = notesEditor->toHtml();
    notesPageFull = false;

    // Re-evaluate fullness and update projection/label
    onNotesTextChanged();
}

void MainWindow::onNotesNextPage()
{
    if (!notesEditor) {
        return;
    }
    if (currentNotesPageIndex >= notesPages.size() - 1) {
        return;
    }

    // Save current page content
    if (currentNotesPageIndex >= 0 && currentNotesPageIndex < notesPages.size()) {
        notesPages[currentNotesPageIndex] = notesEditor->toHtml();
    }

    // Move to next page
    ++currentNotesPageIndex;
    QString html;
    if (currentNotesPageIndex >= 0 && currentNotesPageIndex < notesPages.size()) {
        html = notesPages[currentNotesPageIndex];
    }

    {
        QSignalBlocker blocker(notesEditor);
        notesEditor->setHtml(html);
    }

    notesLastGoodHtml = notesEditor->toHtml();
    notesPageFull = false;

    // Re-evaluate fullness and update projection/label
    onNotesTextChanged();
}

void MainWindow::onNotesAddPage()
{
    if (!notesEditor) {
        return;
    }

    // Save current page content
    if (currentNotesPageIndex >= 0 && currentNotesPageIndex < notesPages.size()) {
        notesPages[currentNotesPageIndex] = notesEditor->toHtml();
    }

    // Append a new empty page and switch to it
    notesPages.append(QString());
    currentNotesPageIndex = notesPages.size() - 1;

    {
        QSignalBlocker blocker(notesEditor);
        notesEditor->clear();
    }

    notesLastGoodHtml = notesEditor->toHtml();
    notesPageFull = false;

    if (notesPageStatusLabel) {
        notesPageStatusLabel->setStyleSheet(QString());
    }

    // Re-evaluate fullness (will also push empty HTML to projection)
    onNotesTextChanged();
}

void MainWindow::projectMediaItem(const QString &path, bool isVideo)
{
    if (projectionCanvas) {
        projectionCanvas->showMedia(path, isVideo);
    }
    
    if (fullscreenProjection) {
        fullscreenProjection->showMedia(path, isVideo);
    }
    
    // Update overlay server for OBS
    if (overlayServer) {
        // Clear text overlays and YouTube when showing media
        overlayServer->updateOverlay("", "");
        overlayServer->updateYouTube("");
        overlayServer->updateMedia(path, isVideo);
        
        // Broadcast media load to OBS via WebSocket
        qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
        overlayServer->broadcastMediaLoad(path, isVideo, timestamp);
        overlayServer->triggerRefresh();
    }
    
    statusBar()->showMessage(QString("Projecting media: %1").arg(QFileInfo(path).fileName()), 3000);
}

void MainWindow::onMediaVideoStarted()
{
    if (!mediaControlsWidget) {
        return;
    }

    mediaControlsWidget->setVisible(true);
    mediaSeekSliderDragging = false;
    if (mediaSeekSlider) {
        mediaSeekSlider->setEnabled(true);
        mediaSeekSlider->setRange(0, 1000);
        mediaSeekSlider->setValue(0);
    }

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    if (projectionCanvas && projectionCanvas->isYouTubeActive()) {
        if (youtubePositionTimer) {
            youtubePositionTimer->start();
        }
    } else {
        if (youtubePositionTimer) {
            youtubePositionTimer->stop();
        }
    }
#endif

    if (mediaVolumeSlider) {
        if (QAudioOutput *output = projectionCanvas->mediaAudioOutput()) {
            mediaVolumeSlider->blockSignals(true);
            mediaVolumeSlider->setValue(qBound(0, static_cast<int>(output->volume() * 100.0 + 0.5), 100));
            mediaVolumeSlider->blockSignals(false);
        }
    }

    if (QMediaPlayer *player = projectionCanvas->mediaPlayer()) {
        onMediaDurationChanged(player->duration());
        onMediaPositionChanged(player->position());
    }

    updateMediaPlayPauseIcon();
}

void MainWindow::onMediaEntryAboutToRemove(const QString &path, bool wasVideo)
{
    Q_UNUSED(wasVideo);

    if (!projectionCanvas) {
        return;
    }

    projectionCanvas->stopMediaPlaybackIfMatches(path);
    
    // Hide media controls when removing media
    if (mediaControlsWidget) {
        mediaControlsWidget->setVisible(false);
    }
    if (mediaSeekSlider) {
        mediaSeekSlider->setEnabled(false);
        mediaSeekSlider->setValue(0);
    }
}

void MainWindow::onMediaEntryRemoved(const QString &path, bool wasVideo)
{
    Q_UNUSED(wasVideo);

    if (!projectionCanvas) {
        return;
    }

    projectionCanvas->stopMediaPlaybackIfMatches(path);
    mediaControlsWidget->setVisible(false);
    if (mediaSeekSlider) {
        mediaSeekSlider->setEnabled(false);
        mediaSeekSlider->setValue(0);
    }
    updateMediaPlayPauseIcon();
}

void MainWindow::onMediaVideoStopped()
{
    if (!mediaControlsWidget) {
        return;
    }

    mediaControlsWidget->setVisible(false);
    mediaSeekSliderDragging = false;
    if (mediaSeekSlider) {
        mediaSeekSlider->setEnabled(false);
        mediaSeekSlider->setRange(0, 1000);
        mediaSeekSlider->setValue(0);
    }
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    if (youtubePositionTimer) {
        youtubePositionTimer->stop();
    }
#endif
    updateMediaPlayPauseIcon();
}

void MainWindow::onMediaPlayPauseClicked()
{
    if (!projectionCanvas) {
        return;
    }

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    // If a YouTube video is active, drive YouTube instead of QMediaPlayer
    if (projectionCanvas->isYouTubeActive()) {
        youtubePlaying = !youtubePlaying;
        projectionCanvas->playPauseYouTube(youtubePlaying);
        if (fullscreenProjection) {
            fullscreenProjection->playPauseYouTube(youtubePlaying);
        }
        if (overlayServer) {
            overlayServer->broadcastYouTubePlayPause(youtubePlaying);
        }
        return;
    }
#endif

    QMediaPlayer *player = projectionCanvas->mediaPlayer();
    if (!player) {
        return;
    }

    bool willBePlaying = (player->playbackState() != QMediaPlayer::PlayingState);
    
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        player->pause();
    } else {
        player->play();
    }
    
    // Broadcast play/pause to OBS via WebSocket
    if (overlayServer) {
        overlayServer->broadcastMediaPlayPause(willBePlaying);
    }
}

void MainWindow::onMediaPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    Q_UNUSED(state);
    if (!projectionCanvas || !projectionCanvas->isMediaVideoActive()) {
        return;
    }
    updateMediaPlayPauseIcon();
}

void MainWindow::onMediaPositionChanged(qint64 position)
{
    if (!mediaControlsWidget || !mediaControlsWidget->isVisible() || mediaSeekSliderDragging || !mediaSeekSlider) {
        return;
    }

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    if (projectionCanvas->isYouTubeActive()) {
        // For YouTube we don't know the absolute duration here, but we can
        // still reflect slider changes when the user scrubs. Position updates
        // from QMediaPlayer are not relevant while YouTube is active.
        return;
    }
#endif

    qint64 duration = projectionCanvas->mediaPlayer() ? projectionCanvas->mediaPlayer()->duration() : 0;
    if (duration <= 0) {
        mediaSeekSlider->blockSignals(true);
        mediaSeekSlider->setValue(0);
        mediaSeekSlider->blockSignals(false);
        return;
    }

    double ratio = static_cast<double>(position) / static_cast<double>(duration);
    int sliderValue = qBound(0, static_cast<int>(ratio * 1000.0 + 0.5), 1000);
    mediaSeekSlider->blockSignals(true);
    mediaSeekSlider->setValue(sliderValue);
    mediaSeekSlider->blockSignals(false);
}

void MainWindow::onMediaDurationChanged(qint64 duration)
{
    if (!mediaControlsWidget || !mediaControlsWidget->isVisible() || !mediaSeekSlider) {
        return;
    }

    mediaSeekSlider->setEnabled(duration > 0);
    if (duration <= 0) {
        mediaSeekSlider->setValue(0);
    }
}

void MainWindow::onMediaSeekSliderPressed()
{
    mediaSeekSliderDragging = true;
}

void MainWindow::onMediaSeekSliderReleased()
{
    mediaSeekSliderDragging = false;

    if (!projectionCanvas || !mediaSeekSlider) {
        return;
    }

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    if (projectionCanvas->isYouTubeActive()) {
        // Map slider value [0,1000] to ratio [0,1]
        double ratio = static_cast<double>(mediaSeekSlider->value()) / 1000.0;
        projectionCanvas->seekYouTubeByRatio(ratio);
        if (fullscreenProjection) {
            fullscreenProjection->seekYouTubeByRatio(ratio);
        }
        if (overlayServer) {
            overlayServer->broadcastYouTubeSeek(ratio);
        }
        return;
    }
#endif

    QMediaPlayer *player = projectionCanvas->mediaPlayer();
    if (!player) {
        return;
    }

    qint64 duration = player->duration();
    if (duration <= 0) {
        return;
    }

    qint64 positionMs = static_cast<qint64>(mediaSeekSlider->value()) * duration / 1000;
    player->setPosition(positionMs);
    
    // Broadcast seek to OBS via WebSocket
    if (overlayServer) {
        overlayServer->broadcastMediaSeek(positionMs);
    }
}

void MainWindow::onMediaSeekSliderMoved(int value)
{
    if (!projectionCanvas) {
        return;
    }

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    if (projectionCanvas->isYouTubeActive()) {
        // While dragging for YouTube, continuously send ratio updates
        double ratio = static_cast<double>(value) / 1000.0;
        projectionCanvas->seekYouTubeByRatio(ratio);
        if (fullscreenProjection) {
            fullscreenProjection->seekYouTubeByRatio(ratio);
        }
        if (overlayServer) {
            overlayServer->broadcastYouTubeSeek(ratio);
        }
        return;
    }
#endif

    QMediaPlayer *player = projectionCanvas->mediaPlayer();
    if (!player) {
        return;
    }

    qint64 duration = player->duration();
    if (duration <= 0) {
        return;
    }

    qint64 positionMs = static_cast<qint64>(value) * duration / 1000;
    player->setPosition(positionMs);
    
    // Broadcast seek to OBS via WebSocket while dragging
    if (overlayServer) {
        overlayServer->broadcastMediaSeek(positionMs);
    }
}

void MainWindow::onMediaVolumeChanged(int value)
{
    if (!projectionCanvas) {
        return;
    }

    const double volume01 = qBound(0.0, static_cast<double>(value) / 100.0, 1.0);

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    // When YouTube is active, drive the YouTube player volume instead of (or in
    // addition to) the QMediaPlayer audio output. OBS overlay stays muted.
    if (projectionCanvas->isYouTubeActive()) {
        projectionCanvas->setYouTubeVolume(volume01);
        return;
    }
#endif

    if (QAudioOutput *output = projectionCanvas->mediaAudioOutput()) {
        output->setVolume(volume01);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (playlistPanel->hasUnsavedChanges()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Save Changes?",
            "Do you want to save changes before closing?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
        );
        
        if (reply == QMessageBox::Yes) {
            savePlaylist();
        } else if (reply == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    
    saveSettings();

    if (fullscreenProjection) {
        fullscreenProjection->close();
        fullscreenProjection->deleteLater();
        fullscreenProjection = nullptr;
    }

    event->accept();
}
