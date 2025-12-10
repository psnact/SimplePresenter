#include "PowerPointPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QListWidget>
#include <QSlider>
#include <QCheckBox>
#include <QProgressBar>
#include <QProcess>
#include <QImage>
#include <QCoreApplication>
#include <QProgressDialog>
#include <QApplication>

#ifdef SIMPLEPRESENTER_HAVE_QTPDF
#include <QPdfDocument>
#endif

#ifdef SIMPLEPRESENTER_HAVE_ACTIVEQT
#include <QAxObject>
#endif

PowerPointPanel::PowerPointPanel(QWidget *parent)
    : QWidget(parent)
    , openButton(nullptr)
    , startButton(nullptr)
    , prevButton(nullptr)
    , nextButton(nullptr)
    , endButton(nullptr)
    , statusLabel(nullptr)
    , thumbnailList(nullptr)
    , thumbnailZoomSlider(nullptr)
    , fadeCheckBox(nullptr)
    , loadingProgress(nullptr)
    , pptApp(nullptr)
    , pptPresentation(nullptr)
    , currentSlideIndex(0)
    , totalSlideCount(0)
{
    setupUI();

#ifndef SIMPLEPRESENTER_HAVE_ACTIVEQT
#ifdef Q_OS_MACOS
    libreOfficeExecutable = findLibreOfficeExecutable();
    if (libreOfficeExecutable.isEmpty()) {
        if (statusLabel) {
            statusLabel->setText("PowerPoint support on macOS requires LibreOffice. Please install LibreOffice to enable slide conversion.");
        }
        if (openButton)  openButton->setEnabled(false);
        if (startButton) startButton->setEnabled(false);
        if (prevButton)  prevButton->setEnabled(false);
        if (nextButton)  nextButton->setEnabled(false);
        if (endButton)   endButton->setEnabled(false);
    } else {
        if (statusLabel) {
            statusLabel->setText("Use 'Open Presentation...' to convert slides via LibreOffice.");
        }
        if (openButton)  openButton->setEnabled(true);
        if (startButton) startButton->setEnabled(false);
        if (prevButton)  prevButton->setEnabled(false);
        if (nextButton)  nextButton->setEnabled(false);
        if (endButton)   endButton->setEnabled(false);
    }
#else
    if (statusLabel) {
        statusLabel->setText("PowerPoint integration requires Qt ActiveQt (AxContainer), which is not available in this build.");
    }
    if (openButton)  openButton->setEnabled(false);
    if (startButton) startButton->setEnabled(false);
    if (prevButton)  prevButton->setEnabled(false);
    if (nextButton)  nextButton->setEnabled(false);
    if (endButton)   endButton->setEnabled(false);
#endif
#endif
}

PowerPointPanel::~PowerPointPanel()
{
}

void PowerPointPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *title = new QLabel("PowerPoint Presentations", this);
    QFont f = title->font();
    f.setBold(true);
    title->setFont(f);
    mainLayout->addWidget(title);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    openButton = new QPushButton("Open Presentation...", this);
    startButton = new QPushButton("Start", this);
    prevButton = new QPushButton("Prev", this);
    nextButton = new QPushButton("Next", this);
    endButton = new QPushButton("End", this);

    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(prevButton);
    buttonLayout->addWidget(nextButton);
    buttonLayout->addWidget(endButton);
    buttonLayout->addStretch();

    loadingProgress = new QProgressBar(this);
    loadingProgress->setRange(0, 0);
    loadingProgress->setFixedWidth(80);
    loadingProgress->setTextVisible(false);
    loadingProgress->setVisible(false);
    buttonLayout->addWidget(loadingProgress);

    mainLayout->addLayout(buttonLayout);

    statusLabel = new QLabel("No presentation loaded", this);
    mainLayout->addWidget(statusLabel);

    QHBoxLayout *zoomLayout = new QHBoxLayout();
    QLabel *zoomLabel = new QLabel("Thumbnail size:", this);
    thumbnailZoomSlider = new QSlider(Qt::Horizontal, this);
    thumbnailZoomSlider->setRange(50, 200);
    thumbnailZoomSlider->setValue(100);
    thumbnailZoomSlider->setFixedWidth(150);
    zoomLayout->addWidget(zoomLabel);
    zoomLayout->addWidget(thumbnailZoomSlider);
    fadeCheckBox = new QCheckBox("Fade transition", this);
    fadeCheckBox->setToolTip("Fade between slides when changing");
    zoomLayout->addWidget(fadeCheckBox);
    zoomLayout->addStretch();
    mainLayout->addLayout(zoomLayout);

    thumbnailList = new QListWidget(this);
    thumbnailList->setViewMode(QListView::IconMode);
    thumbnailList->setFlow(QListView::LeftToRight);
    thumbnailList->setWrapping(true);
    thumbnailList->setResizeMode(QListView::Adjust);
    thumbnailList->setMovement(QListView::Static);
    thumbnailList->setSelectionMode(QAbstractItemView::SingleSelection);
    thumbnailList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    thumbnailList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    thumbnailList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    thumbnailList->setSpacing(8);
    thumbnailList->setIconSize(QSize(160, 90));
    mainLayout->addWidget(thumbnailList, 1);

    connect(openButton, &QPushButton::clicked,
            this, &PowerPointPanel::onOpenPresentation);
    connect(startButton, &QPushButton::clicked,
            this, &PowerPointPanel::onStartSlideShow);
    connect(prevButton, &QPushButton::clicked,
            this, &PowerPointPanel::onPreviousSlide);
    connect(nextButton, &QPushButton::clicked,
            this, &PowerPointPanel::onNextSlide);
    connect(endButton, &QPushButton::clicked,
            this, &PowerPointPanel::onEndSlideShow);

    connect(thumbnailZoomSlider, &QSlider::valueChanged,
            this, [this](int value) {
                if (!thumbnailList) {
                    return;
                }
                int baseWidth = 160;
                int baseHeight = 90;
                double factor = value / 100.0;
                QSize size(qMax(80, int(baseWidth * factor)), qMax(45, int(baseHeight * factor)));
                thumbnailList->setIconSize(size);
            });

    connect(thumbnailList, &QListWidget::itemClicked,
            this, [this](QListWidgetItem *item) {
                if (!item) {
                    return;
                }
                bool ok = false;
                int slideIndex = item->data(Qt::UserRole + 1).toInt(&ok);
                if (!ok || slideIndex <= 0) {
                    return;
                }
#ifdef SIMPLEPRESENTER_HAVE_ACTIVEQT
                if (!slideImagePaths.isEmpty()) {
                    if (slideIndex < 1 || slideIndex > slideImagePaths.size()) {
                        return;
                    }
                    currentSlideIndex = slideIndex;
                    totalSlideCount = slideImagePaths.size();
                    if (prevButton) {
                        prevButton->setEnabled(currentSlideIndex > 1);
                    }
                    if (nextButton) {
                        nextButton->setEnabled(currentSlideIndex < totalSlideCount);
                    }
                    if (endButton) {
                        endButton->setEnabled(totalSlideCount > 0);
                    }
                    if (statusLabel) {
                        statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(totalSlideCount));
                    }
                    bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
                    emit slideImageAvailable(slideImagePaths.value(currentSlideIndex - 1), useFade);
                } else {
                    if (!pptPresentation) {
                        return;
                    }
                    currentSlideIndex = slideIndex;
                    if (totalSlideCount <= 0) {
                        QAxObject *slides = pptPresentation->querySubObject("Slides");
                        if (slides && !slides->isNull()) {
                            totalSlideCount = slides->property("Count").toInt();
                        }
                        delete slides;
                    }
                    prevButton->setEnabled(currentSlideIndex > 1);
                    nextButton->setEnabled(totalSlideCount > 0 && currentSlideIndex < totalSlideCount);
                    endButton->setEnabled(totalSlideCount > 0);
                    if (totalSlideCount > 0) {
                        statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(totalSlideCount));
                    }
                    QString imagePath = exportCurrentSlideAsImage();
                    if (!imagePath.isEmpty()) {
                        bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
                        emit slideImageAvailable(imagePath, useFade);
                    }
                }
#else
                if (slideIndex < 1 || slideIndex > slideImagePaths.size()) {
                    return;
                }
                currentSlideIndex = slideIndex;
                totalSlideCount = slideImagePaths.size();
                if (prevButton) {
                    prevButton->setEnabled(currentSlideIndex > 1);
                }
                if (nextButton) {
                    nextButton->setEnabled(currentSlideIndex < totalSlideCount);
                }
                if (endButton) {
                    endButton->setEnabled(totalSlideCount > 0);
                }
                if (statusLabel) {
                    statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(totalSlideCount));
                }
                if (!slideImagePaths.isEmpty()) {
                    bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
                    emit slideImageAvailable(slideImagePaths.value(currentSlideIndex - 1), useFade);
                }
#endif
            });

    startButton->setEnabled(false);
    prevButton->setEnabled(false);
    nextButton->setEnabled(false);
    endButton->setEnabled(false);
}

void PowerPointPanel::setLoading(bool loading)
{
    if (!loadingProgress) {
        return;
    }
    loadingProgress->setVisible(loading);
    if (loading) {
        loadingProgress->setRange(0, 0);
    } else {
        loadingProgress->setRange(0, 1);
        loadingProgress->setValue(0);
    }
}

void PowerPointPanel::ensurePowerPoint()
{
#ifndef SIMPLEPRESENTER_HAVE_ACTIVEQT
    Q_UNUSED(pptApp);
    if (statusLabel) {
        statusLabel->setText("PowerPoint integration is not available in this build.");
    }
    return;
#else
    if (pptApp) {
        return;
    }

#ifdef Q_OS_WIN
    // On Windows, try Microsoft PowerPoint (COM) first, then fall back to
    // LibreOffice-based conversion if COM is not available.
    QAxObject *app = new QAxObject("PowerPoint.Application", this);
    if (!app || app->isNull()) {
        delete app;
        pptApp = nullptr;

        // Attempt LibreOffice-based fallback
        if (libreOfficeExecutable.isEmpty()) {
            libreOfficeExecutable = findLibreOfficeExecutable();
        }

        if (!libreOfficeExecutable.isEmpty()) {
            if (statusLabel) {
                statusLabel->setText("Microsoft PowerPoint not available; using LibreOffice for slide conversion.");
            }
            if (openButton)  openButton->setEnabled(true);
            if (startButton) startButton->setEnabled(false);
            if (prevButton)  prevButton->setEnabled(false);
            if (nextButton)  nextButton->setEnabled(false);
            if (endButton)   endButton->setEnabled(false);
        } else {
            if (statusLabel) {
                statusLabel->setText("PowerPoint not available (requires Microsoft PowerPoint or LibreOffice).");
            }
            if (openButton)  openButton->setEnabled(false);
            if (startButton) startButton->setEnabled(false);
            if (prevButton)  prevButton->setEnabled(false);
            if (nextButton)  nextButton->setEnabled(false);
            if (endButton)   endButton->setEnabled(false);
        }

        return;
    }

    pptApp = app;
    // Run PowerPoint in the background without showing its main window
    pptApp->setProperty("Visible", false);
#else
    pptApp = new QAxObject("PowerPoint.Application", this);
    if (!pptApp || pptApp->isNull()) {
        if (statusLabel) {
            statusLabel->setText("PowerPoint not available (requires Microsoft PowerPoint)");
        }
        delete pptApp;
        pptApp = nullptr;
        return;
    }

    // Run PowerPoint in the background without showing its main window
    pptApp->setProperty("Visible", false);
#endif
#endif
}

bool PowerPointPanel::hasActiveSlideShow() const
{
#ifndef SIMPLEPRESENTER_HAVE_ACTIVEQT
    return false;
#else
    if (!pptApp) {
        return false;
    }

    QAxObject slideShowWindows(pptApp->querySubObject("SlideShowWindows"));
    if (slideShowWindows.isNull()) {
        return false;
    }

    int count = slideShowWindows.property("Count").toInt();
    return count > 0;
#endif
}

QString PowerPointPanel::exportCurrentSlideAsImage()
{
#ifndef SIMPLEPRESENTER_HAVE_ACTIVEQT
    return QString();
#else
    if (!pptPresentation || currentSlideIndex <= 0) {
        return QString();
    }

    QAxObject *slides = pptPresentation->querySubObject("Slides");
    if (!slides || slides->isNull()) {
        delete slides;
        return QString();
    }

    int count = slides->property("Count").toInt();
    if (count <= 0 || currentSlideIndex > count) {
        delete slides;
        return QString();
    }

    QAxObject *slide = slides->querySubObject("Item(int)", currentSlideIndex);
    if (!slide || slide->isNull()) {
        statusLabel->setText("Unable to access current slide for export");
        delete slides;
        return QString();
    }

    // Build export directory under AppDataLocation/slides
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        delete slide;
        return QString();
    }

    QDir dir(baseDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    dir.mkpath("slides");
    dir.cd("slides");

    QString baseName = QFileInfo(currentPresentationPath).completeBaseName();
    if (baseName.isEmpty()) {
        baseName = QStringLiteral("presentation");
    }

    QString targetPath = dir.filePath(baseName + QStringLiteral("_current.png"));
    QString nativePath = QDir::toNativeSeparators(targetPath);

    // Export the slide as PNG at 1920x1080 while 'slides' and 'slide' are still alive
    slide->dynamicCall("Export(const QString&, const QString&, int, int)",
                       nativePath,
                       QStringLiteral("PNG"),
                       1920,
                       1080);

    delete slides; // this also deletes 'slide' since it's a child

    if (!QFileInfo::exists(nativePath)) {
        return QString();
    }

    return nativePath;
#endif
}

void PowerPointPanel::rebuildThumbnails()
{
#ifndef SIMPLEPRESENTER_HAVE_ACTIVEQT
    if (thumbnailList) {
        thumbnailList->clear();
    }
    totalSlideCount = 0;
    return;
#else
    if (!thumbnailList) {
        return;
    }

    thumbnailList->clear();
    totalSlideCount = 0;

    if (!pptPresentation) {
        return;
    }

    QAxObject *slides = pptPresentation->querySubObject("Slides");
    if (!slides || slides->isNull()) {
        delete slides;
        return;
    }

    int count = slides->property("Count").toInt();
    if (count <= 0) {
        delete slides;
        return;
    }

    totalSlideCount = count;

    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        delete slides;
        return;
    }

    QDir dir(baseDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    dir.mkpath("slides");
    dir.cd("slides");

    QString baseName = QFileInfo(currentPresentationPath).completeBaseName();
    if (baseName.isEmpty()) {
        baseName = QStringLiteral("presentation");
    }

    for (int i = 1; i <= count; ++i) {
        QAxObject *slide = slides->querySubObject("Item(int)", i);
        if (!slide || slide->isNull()) {
            delete slide;
            continue;
        }

        QString thumbName = QStringLiteral("%1_thumb_%2.png").arg(baseName).arg(i);
        QString thumbPath = dir.filePath(thumbName);
        QString thumbNativePath = QDir::toNativeSeparators(thumbPath);

        slide->dynamicCall("Export(const QString&, const QString&, int, int)",
                           thumbNativePath,
                           QStringLiteral("PNG"),
                           640,
                           360);

        delete slide;

        if (!QFileInfo::exists(thumbNativePath)) {
            continue;
        }

        QIcon icon(thumbNativePath);
        QListWidgetItem *item = new QListWidgetItem(icon, QString("Slide %1").arg(i), thumbnailList);
        item->setData(Qt::UserRole + 1, i);
        item->setToolTip(QString("Slide %1").arg(i));
    }

    delete slides;

    if (thumbnailList->count() > 0) {
        thumbnailList->setCurrentRow(0);
    }
#endif
}

void PowerPointPanel::onOpenPresentation()
{
#ifndef SIMPLEPRESENTER_HAVE_ACTIVEQT
#ifdef Q_OS_MACOS
    if (libreOfficeExecutable.isEmpty()) {
        if (statusLabel) {
            statusLabel->setText("LibreOffice not found. Install LibreOffice to enable PowerPoint conversion.");
        }
        return;
    }

    QFileDialog dialog(this, "Open PowerPoint Presentation");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter("PowerPoint Files (*.ppt *.pptx);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QString fileName = dialog.selectedFiles().value(0);
    if (fileName.isEmpty()) {
        return;
    }

    if (statusLabel) {
        statusLabel->setText("Converting presentation... please wait");
    }

    QProgressDialog progress("Converting presentation...", QString(), 0, 0, window());
    progress.setWindowTitle("Converting presentation...");
    progress.setLabelText("Converting presentation...\nThis may take up to 10-15 seconds.");
    progress.setMinimumWidth(320);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setCancelButton(nullptr);
    progress.setMinimumDuration(0);
    progress.setAutoClose(false);
    progress.setAutoReset(false);
    progress.show();
    progress.raise();
    progress.activateWindow();

    setLoading(true);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    QString outputDir;
    QString errorMessage;
    bool ok = convertPresentationWithLibreOffice(fileName, outputDir, errorMessage);

    progress.close();

    if (!ok) {
        setLoading(false);
        slideImagePaths.clear();
        if (thumbnailList) {
            thumbnailList->clear();
        }
        if (statusLabel) {
            statusLabel->setText(errorMessage.isEmpty() ? QStringLiteral("Failed to convert presentation with LibreOffice.")
                                                       : errorMessage);
        }
        if (startButton) startButton->setEnabled(false);
        if (prevButton)  prevButton->setEnabled(false);
        if (nextButton)  nextButton->setEnabled(false);
        if (endButton)   endButton->setEnabled(false);
        return;
    }

    setLoading(false);

    currentPresentationPath = fileName;
    currentSlideIndex = 0;
    totalSlideCount = slideImagePaths.size();

    if (thumbnailList) {
        thumbnailList->clear();
        for (int i = 0; i < slideImagePaths.size(); ++i) {
            QIcon icon(slideImagePaths.at(i));
            QListWidgetItem *item = new QListWidgetItem(icon, QString("Slide %1").arg(i + 1), thumbnailList);
            item->setData(Qt::UserRole + 1, i + 1);
            item->setToolTip(QString("Slide %1").arg(i + 1));
        }
    }

    QFileInfo info(fileName);
    if (statusLabel) {
        statusLabel->setText(QString("Converted: %1 (%2 slide%3)")
                                 .arg(info.fileName())
                                 .arg(totalSlideCount)
                                 .arg(totalSlideCount == 1 ? "" : "s"));
    }

    if (startButton) startButton->setEnabled(totalSlideCount > 0);
    if (prevButton)  prevButton->setEnabled(false);
    if (nextButton)  nextButton->setEnabled(false);
    if (endButton)   endButton->setEnabled(false);

    emit presentationOpened(fileName);
#else
    if (statusLabel) {
        statusLabel->setText("PowerPoint integration is not available in this build.");
    }
    return;
#endif
#else
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open PowerPoint Presentation",
        QString(),
        "PowerPoint Files (*.ppt *.pptx);;All Files (*)");

    if (fileName.isEmpty()) {
        return;
    }

    slideImagePaths.clear();

    QString openPath = fileName;

#ifdef Q_OS_WIN
    // Work around path and filename limitations in PowerPoint COM automation by
    // always copying presentations to a shorter, sanitized temporary location
    // before opening them via COM.
    {
        QString tempRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        if (tempRoot.isEmpty()) {
            tempRoot = QDir::tempPath();
        }
        QDir tempDir(tempRoot);
        if (tempDir.mkpath("SimplePresenterPPT") && tempDir.cd("SimplePresenterPPT")) {
            QFileInfo fi(fileName);
            QString base = fi.completeBaseName();
            if (base.isEmpty()) {
                base = QStringLiteral("presentation");
            }

            // Sanitize base name to ASCII-ish safe characters
            QString sanitizedBase;
            sanitizedBase.reserve(base.size());
            for (const QChar &ch : base) {
                if (ch.isLetterOrNumber()) {
                    sanitizedBase.append(ch);
                } else if (ch == '_' || ch == '-') {
                    sanitizedBase.append(ch);
                } else {
                    sanitizedBase.append('_');
                }
            }
            if (sanitizedBase.isEmpty()) {
                sanitizedBase = QStringLiteral("presentation");
            }

            QString ext = fi.completeSuffix();
            if (!ext.isEmpty()) {
                ext.prepend('.');
            }

            QString targetPath = tempDir.filePath(sanitizedBase + ext);
            int counter = 1;
            while (QFile::exists(targetPath) && counter < 1000) {
                targetPath = tempDir.filePath(QStringLiteral("%1_%2%3").arg(sanitizedBase).arg(counter).arg(ext));
                ++counter;
            }

            if (QFile::copy(fileName, targetPath)) {
                openPath = targetPath;
            }
        }
    }
#endif

    ensurePowerPoint();

#ifdef Q_OS_WIN
    // If Microsoft PowerPoint (COM) is not available but LibreOffice is,
    // fall back to the LibreOffice-based conversion pipeline on Windows.
    if (!pptApp && !libreOfficeExecutable.isEmpty()) {
        if (statusLabel) {
            statusLabel->setText("Converting presentation... please wait");
        }

        QProgressDialog progress("Converting presentation...", QString(), 0, 0, window());
        progress.setWindowTitle("Converting presentation...");
        progress.setLabelText("Converting presentation...\nThis may take up to 10-15 seconds.");
        progress.setMinimumWidth(320);
        progress.setWindowModality(Qt::ApplicationModal);
        progress.setCancelButton(nullptr);
        progress.setMinimumDuration(0);
        progress.setAutoClose(false);
        progress.setAutoReset(false);
        progress.show();
        progress.raise();
        progress.activateWindow();

        setLoading(true);
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        QString outputDir;
        QString errorMessage;
        bool ok = convertPresentationWithLibreOffice(fileName, outputDir, errorMessage);

        progress.close();

        if (!ok) {
            setLoading(false);
            slideImagePaths.clear();
            if (thumbnailList) {
                thumbnailList->clear();
            }
            if (statusLabel) {
                statusLabel->setText(errorMessage.isEmpty() ? QStringLiteral("Failed to convert presentation with LibreOffice.")
                                                           : errorMessage);
            }
            if (startButton) startButton->setEnabled(false);
            if (prevButton)  prevButton->setEnabled(false);
            if (nextButton)  nextButton->setEnabled(false);
            if (endButton)   endButton->setEnabled(false);
            return;
        }

        setLoading(false);

        currentPresentationPath = fileName;
        currentSlideIndex = 0;
        totalSlideCount = slideImagePaths.size();

        if (thumbnailList) {
            thumbnailList->clear();
            for (int i = 0; i < slideImagePaths.size(); ++i) {
                QIcon icon(slideImagePaths.at(i));
                QListWidgetItem *item = new QListWidgetItem(icon, QString("Slide %1").arg(i + 1), thumbnailList);
                item->setData(Qt::UserRole + 1, i + 1);
                item->setToolTip(QString("Slide %1").arg(i + 1));
            }
        }

        QFileInfo info(fileName);
        if (statusLabel) {
            statusLabel->setText(QString("Converted: %1 (%2 slide%3)")
                                     .arg(info.fileName())
                                     .arg(totalSlideCount)
                                     .arg(totalSlideCount == 1 ? "" : "s"));
        }

        if (startButton) startButton->setEnabled(totalSlideCount > 0);
        if (prevButton)  prevButton->setEnabled(false);
        if (nextButton)  nextButton->setEnabled(false);
        if (endButton)   endButton->setEnabled(false);

        emit presentationOpened(fileName);
        return;
    }
#endif

    if (!pptApp) {
        return;
    }

    if (pptPresentation) {
        pptPresentation->dynamicCall("Close()");
        delete pptPresentation;
        pptPresentation = nullptr;
    }

    QAxObject *presentations = pptApp->querySubObject("Presentations");
    if (!presentations || presentations->isNull()) {
        statusLabel->setText("Could not access PowerPoint presentations collection");
        delete presentations;
        return;
    }

    if (statusLabel) {
        statusLabel->setText("Loading presentation... please wait");
    }

    QProgressDialog progress("Loading presentation...", QString(), 0, 0, window());
    progress.setWindowTitle("Loading presentation...");
    progress.setLabelText("Loading presentation...\nThis may take up to 10-15 seconds.");
    progress.setMinimumWidth(320);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setCancelButton(nullptr);
    progress.setMinimumDuration(0);
    progress.setAutoClose(false);
    progress.setAutoReset(false);
    progress.show();
    progress.raise();
    progress.activateWindow();

    setLoading(true);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // Open(fileName, ReadOnly, Untitled, WithWindow)
    // Use ReadOnly=true so we can open files that are already in use in PowerPoint.
    // Pass a native Windows path but do NOT show the PowerPoint window, so the
    // app stays invisible while we automate exports.
    QString nativeOpenPath = QDir::toNativeSeparators(openPath);
    pptPresentation = presentations->querySubObject(
        "Open(const QString&, bool, bool, bool)",
        nativeOpenPath,
        true,
        false,
        false);
    if (pptPresentation) {
        // Ensure pptPresentation is not deleted when we delete 'presentations'
        pptPresentation->setParent(this);
    }

    delete presentations;

    if (!pptPresentation || pptPresentation->isNull()) {
        setLoading(false);
        progress.close();

        QFileInfo info(fileName);
        statusLabel->setText(QString("Failed to open: %1 (check if it is locked, password-protected, or corrupted)")
                             .arg(info.fileName()));
        if (pptPresentation) {
            delete pptPresentation;
            pptPresentation = nullptr;
        }
        return;
    }

    currentPresentationPath = fileName;
    currentSlideIndex = 0;
    QFileInfo info(fileName);
    statusLabel->setText(QString("Loaded: %1").arg(info.fileName()));

    rebuildThumbnails();

    setLoading(false);
    progress.close();

    startButton->setEnabled(true);
    prevButton->setEnabled(false);
    nextButton->setEnabled(false);
    endButton->setEnabled(false);

    emit presentationOpened(fileName);
#endif
}

void PowerPointPanel::onStartSlideShow()
{
#ifdef SIMPLEPRESENTER_HAVE_ACTIVEQT
    if (!slideImagePaths.isEmpty()) {
        if (slideImagePaths.isEmpty()) {
            if (statusLabel) {
                statusLabel->setText("No slides available. Open a presentation first.");
            }
            return;
        }

        totalSlideCount = slideImagePaths.size();
        currentSlideIndex = 1;

        if (startButton) startButton->setEnabled(false);
        if (prevButton)  prevButton->setEnabled(false);
        if (nextButton)  nextButton->setEnabled(totalSlideCount > 1);
        if (endButton)   endButton->setEnabled(true);

        if (statusLabel) {
            statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(totalSlideCount));
        }

        if (thumbnailList) {
            for (int row = 0; row < thumbnailList->count(); ++row) {
                QListWidgetItem *item = thumbnailList->item(row);
                if (!item) {
                    continue;
                }
                bool ok = false;
                int slideIndex = item->data(Qt::UserRole + 1).toInt(&ok);
                bool selected = ok && slideIndex == currentSlideIndex;
                item->setSelected(selected);
                if (selected) {
                    thumbnailList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
                }
            }
        }

        bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
        emit slideImageAvailable(slideImagePaths.value(0), useFade);
        return;
    }

    if (!pptPresentation) {
        return;
    }

    setLoading(true);

    QAxObject *slides = pptPresentation->querySubObject("Slides");
    if (!slides || slides->isNull()) {
        delete slides;
        statusLabel->setText("No slides found in presentation");
        setLoading(false);
        return;
    }

    int count = slides->property("Count").toInt();
    delete slides;

    if (count <= 0) {
        statusLabel->setText("No slides found in presentation");
        setLoading(false);
        return;
    }

    currentSlideIndex = 1;
    totalSlideCount = count;

    startButton->setEnabled(false);
    prevButton->setEnabled(false);
    nextButton->setEnabled(count > 1);
    endButton->setEnabled(true);
    statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(count));

    if (thumbnailList) {
        for (int row = 0; row < thumbnailList->count(); ++row) {
            QListWidgetItem *item = thumbnailList->item(row);
            if (!item) {
                continue;
            }
            bool ok = false;
            int slideIndex = item->data(Qt::UserRole + 1).toInt(&ok);
            bool selected = ok && slideIndex == currentSlideIndex;
            item->setSelected(selected);
            if (selected) {
                thumbnailList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            }
        }
    }

    QString imagePath = exportCurrentSlideAsImage();
    if (!imagePath.isEmpty()) {
        bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
        emit slideImageAvailable(imagePath, useFade);
    }
    setLoading(false);
#else
    if (slideImagePaths.isEmpty()) {
        if (statusLabel) {
            statusLabel->setText("No slides available. Open a presentation first.");
        }
        return;
    }

    totalSlideCount = slideImagePaths.size();
    currentSlideIndex = 1;

    if (startButton) startButton->setEnabled(false);
    if (prevButton)  prevButton->setEnabled(false);
    if (nextButton)  nextButton->setEnabled(totalSlideCount > 1);
    if (endButton)   endButton->setEnabled(true);

    if (statusLabel) {
        statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(totalSlideCount));
    }

    if (thumbnailList) {
        for (int row = 0; row < thumbnailList->count(); ++row) {
            QListWidgetItem *item = thumbnailList->item(row);
            if (!item) {
                continue;
            }
            bool ok = false;
            int slideIndex = item->data(Qt::UserRole + 1).toInt(&ok);
            bool selected = ok && slideIndex == currentSlideIndex;
            item->setSelected(selected);
            if (selected) {
                thumbnailList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            }
        }
    }

    bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
    emit slideImageAvailable(slideImagePaths.value(0), useFade);
#endif
}

void PowerPointPanel::onNextSlide()
{
#ifdef SIMPLEPRESENTER_HAVE_ACTIVEQT
    if (!slideImagePaths.isEmpty()) {
        if (slideImagePaths.isEmpty() || currentSlideIndex <= 0) {
            return;
        }

        int count = slideImagePaths.size();
        if (currentSlideIndex >= count) {
            return;
        }

        ++currentSlideIndex;
        totalSlideCount = count;

        if (prevButton) prevButton->setEnabled(currentSlideIndex > 1);
        if (nextButton) nextButton->setEnabled(currentSlideIndex < count);
        if (endButton)  endButton->setEnabled(true);
        if (statusLabel) {
            statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(count));
        }

        if (thumbnailList) {
            for (int row = 0; row < thumbnailList->count(); ++row) {
                QListWidgetItem *item = thumbnailList->item(row);
                if (!item) {
                    continue;
                }
                bool ok = false;
                int slideIndex = item->data(Qt::UserRole + 1).toInt(&ok);
                bool selected = ok && slideIndex == currentSlideIndex;
                item->setSelected(selected);
                if (selected) {
                    thumbnailList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
                }
            }
        }

        bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
        emit slideImageAvailable(slideImagePaths.value(currentSlideIndex - 1), useFade);
        return;
    }

    if (!pptPresentation || currentSlideIndex <= 0) {
        return;
    }

    QAxObject *slides = pptPresentation->querySubObject("Slides");
    if (!slides || slides->isNull()) {
        delete slides;
        return;
    }

    int count = slides->property("Count").toInt();
    delete slides;

    if (count <= 0 || currentSlideIndex >= count) {
        return;
    }

    ++currentSlideIndex;

    totalSlideCount = count;

    prevButton->setEnabled(currentSlideIndex > 1);
    nextButton->setEnabled(currentSlideIndex < count);
    endButton->setEnabled(true);
    statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(count));

    totalSlideCount = count;

    if (thumbnailList) {
        for (int row = 0; row < thumbnailList->count(); ++row) {
            QListWidgetItem *item = thumbnailList->item(row);
            if (!item) {
                continue;
            }
            bool ok = false;
            int slideIndex = item->data(Qt::UserRole + 1).toInt(&ok);
            bool selected = ok && slideIndex == currentSlideIndex;
            item->setSelected(selected);
            if (selected) {
                thumbnailList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            }
        }
    }

    QString imagePath = exportCurrentSlideAsImage();
    if (!imagePath.isEmpty()) {
        bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
        emit slideImageAvailable(imagePath, useFade);
    }
#else
    if (slideImagePaths.isEmpty() || currentSlideIndex <= 0) {
        return;
    }

    int count = slideImagePaths.size();
    if (currentSlideIndex >= count) {
        return;
    }

    ++currentSlideIndex;
    totalSlideCount = count;

    if (prevButton) prevButton->setEnabled(currentSlideIndex > 1);
    if (nextButton) nextButton->setEnabled(currentSlideIndex < count);
    if (endButton)  endButton->setEnabled(true);
    if (statusLabel) {
        statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(count));
    }

    if (thumbnailList) {
        for (int row = 0; row < thumbnailList->count(); ++row) {
            QListWidgetItem *item = thumbnailList->item(row);
            if (!item) {
                continue;
            }
            bool ok = false;
            int slideIndex = item->data(Qt::UserRole + 1).toInt(&ok);
            bool selected = ok && slideIndex == currentSlideIndex;
            item->setSelected(selected);
            if (selected) {
                thumbnailList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            }
        }
    }

    bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
    emit slideImageAvailable(slideImagePaths.value(currentSlideIndex - 1), useFade);
#endif
}

void PowerPointPanel::onPreviousSlide()
{
#ifdef SIMPLEPRESENTER_HAVE_ACTIVEQT
    if (!slideImagePaths.isEmpty()) {
        if (slideImagePaths.isEmpty() || currentSlideIndex <= 1) {
            return;
        }

        int count = slideImagePaths.size();
        if (count <= 0) {
            return;
        }

        --currentSlideIndex;
        if (currentSlideIndex < 1) {
            currentSlideIndex = 1;
        }

        if (prevButton) prevButton->setEnabled(currentSlideIndex > 1);
        if (nextButton) nextButton->setEnabled(currentSlideIndex < count);
        if (endButton)  endButton->setEnabled(true);
        if (statusLabel) {
            statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(count));
        }

        bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
        emit slideImageAvailable(slideImagePaths.value(currentSlideIndex - 1), useFade);
        return;
    }

    if (!pptPresentation || currentSlideIndex <= 1) {
        return;
    }

    QAxObject *slides = pptPresentation->querySubObject("Slides");
    if (!slides || slides->isNull()) {
        delete slides;
        return;
    }

    int count = slides->property("Count").toInt();
    delete slides;

    if (count <= 0) {
        return;
    }

    --currentSlideIndex;
    if (currentSlideIndex < 1) {
        currentSlideIndex = 1;
    }

    prevButton->setEnabled(currentSlideIndex > 1);
    nextButton->setEnabled(currentSlideIndex < count);
    endButton->setEnabled(true);
    statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(count));

    QString imagePath = exportCurrentSlideAsImage();
    if (!imagePath.isEmpty()) {
        bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
        emit slideImageAvailable(imagePath, useFade);
    }
#else
    if (slideImagePaths.isEmpty() || currentSlideIndex <= 1) {
        return;
    }

    int count = slideImagePaths.size();
    if (count <= 0) {
        return;
    }

    --currentSlideIndex;
    if (currentSlideIndex < 1) {
        currentSlideIndex = 1;
    }

    if (prevButton) prevButton->setEnabled(currentSlideIndex > 1);
    if (nextButton) nextButton->setEnabled(currentSlideIndex < count);
    if (endButton)  endButton->setEnabled(true);
    if (statusLabel) {
        statusLabel->setText(QString("Slide %1 of %2").arg(currentSlideIndex).arg(count));
    }

    bool useFade = fadeCheckBox && fadeCheckBox->isChecked();
    emit slideImageAvailable(slideImagePaths.value(currentSlideIndex - 1), useFade);
#endif
}

void PowerPointPanel::onEndSlideShow()
{
#ifdef SIMPLEPRESENTER_HAVE_ACTIVEQT
    currentSlideIndex = 0;

    if (!slideImagePaths.isEmpty()) {
        if (startButton) startButton->setEnabled(!slideImagePaths.isEmpty());
        if (prevButton)  prevButton->setEnabled(false);
        if (nextButton)  nextButton->setEnabled(false);
        if (endButton)   endButton->setEnabled(false);
        if (statusLabel) {
            statusLabel->setText("Slide show stopped");
        }
    } else {
        startButton->setEnabled(pptPresentation != nullptr);
        prevButton->setEnabled(false);
        nextButton->setEnabled(false);
        endButton->setEnabled(false);
        statusLabel->setText("Slide show stopped");
    }
#else
    currentSlideIndex = 0;

    if (startButton) startButton->setEnabled(!slideImagePaths.isEmpty());
    if (prevButton)  prevButton->setEnabled(false);
    if (nextButton)  nextButton->setEnabled(false);
    if (endButton)   endButton->setEnabled(false);
    if (statusLabel) {
        statusLabel->setText("Slide show stopped");
    }
#endif
}

QString PowerPointPanel::findLibreOfficeExecutable() const
{
#if defined(Q_OS_MACOS)
    // Standard macOS bundle location
    QString bundledPath = QStringLiteral("/Applications/LibreOffice.app/Contents/MacOS/soffice");
    QFileInfo bundledInfo(bundledPath);
    if (bundledInfo.exists() && bundledInfo.isFile() && bundledInfo.isExecutable()) {
        return bundledInfo.absoluteFilePath();
    }

    QString fromPath = QStandardPaths::findExecutable(QStringLiteral("soffice"));
    if (!fromPath.isEmpty()) {
        return fromPath;
    }
#elif defined(Q_OS_WIN)
    // Typical Windows installation paths
    const QStringList candidatePaths = {
        QStringLiteral("C:/Program Files/LibreOffice/program/soffice.exe"),
        QStringLiteral("C:/Program Files (x86)/LibreOffice/program/soffice.exe")
    };

    for (const QString &path : candidatePaths) {
        QFileInfo info(path);
        if (info.exists() && info.isFile() && info.isExecutable()) {
            return info.absoluteFilePath();
        }
    }

    QString fromPath = QStandardPaths::findExecutable(QStringLiteral("soffice"));
    if (!fromPath.isEmpty()) {
        return fromPath;
    }
#endif
    return QString();
}

bool PowerPointPanel::convertPresentationWithLibreOffice(const QString &filePath,
                                                         QString &outputDir,
                                                         QString &errorMessage)
{
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    outputDir.clear();
    errorMessage.clear();

    if (libreOfficeExecutable.isEmpty()) {
        errorMessage = QStringLiteral("LibreOffice executable not configured.");
        return false;
    }

    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        errorMessage = QStringLiteral("Presentation file does not exist.");
        return false;
    }

    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        errorMessage = QStringLiteral("Unable to determine application data directory.");
        return false;
    }

    QDir base(baseDir);
    if (!base.exists()) {
        base.mkpath(QStringLiteral("."));
    }
    base.mkpath(QStringLiteral("slides"));
    base.cd(QStringLiteral("slides"));

    QString baseName = fi.completeBaseName();
    if (baseName.isEmpty()) {
        baseName = QStringLiteral("presentation");
    }

    QString targetDir = base.filePath(baseName);
    int counter = 1;
    while (QDir(targetDir).exists() && counter < 1000) {
        targetDir = base.filePath(QStringLiteral("%1_%2").arg(baseName).arg(counter));
        ++counter;
    }
    if (!QDir().mkpath(targetDir)) {
        errorMessage = QStringLiteral("Unable to create output directory for slides.");
        return false;
    }

    outputDir = targetDir;

#ifdef SIMPLEPRESENTER_HAVE_QTPDF
    QProcess process;
    process.setProgram(libreOfficeExecutable);

    QStringList args;
    args << QStringLiteral("--headless")
         << QStringLiteral("--nologo")
         << QStringLiteral("--norestore")
         << QStringLiteral("--convert-to") << QStringLiteral("pdf")
         << QStringLiteral("--outdir") << outputDir
         << filePath;
    process.setArguments(args);
    process.setProcessChannelMode(QProcess::MergedChannels);

    process.start();
    if (!process.waitForStarted(30000)) {
        errorMessage = QStringLiteral("Failed to start LibreOffice process.");
        return false;
    }

    if (!process.waitForFinished(120000)) {
        process.kill();
        process.waitForFinished(5000);
        errorMessage = QStringLiteral("LibreOffice did not finish conversion in time.");
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QString output = QString::fromLocal8Bit(process.readAll());
        if (output.isEmpty()) {
            output = QStringLiteral("Unknown error from LibreOffice.");
        }
        errorMessage = QStringLiteral("LibreOffice conversion failed: %1").arg(output.trimmed());
        return false;
    }

    QDir outDir(outputDir);
    QString pdfPath = outDir.filePath(baseName + QStringLiteral(".pdf"));
    if (!QFileInfo::exists(pdfPath)) {
        errorMessage = QStringLiteral("LibreOffice did not produce a PDF file.");
        return false;
    }

    QPdfDocument document;
    if (document.load(pdfPath) != QPdfDocument::Error::None) {
        errorMessage = QStringLiteral("Failed to load PDF generated by LibreOffice.");
        return false;
    }

    int pageCount = document.pageCount();
    if (pageCount <= 0) {
        errorMessage = QStringLiteral("The generated PDF has no pages.");
        return false;
    }

    slideImagePaths.clear();
    for (int page = 0; page < pageCount; ++page) {
        QImage image = document.render(page, QSize(1920, 1080));
        if (image.isNull()) {
            continue;
        }
        QString imageName = QStringLiteral("%1_%2.png").arg(baseName).arg(page + 1);
        QString imagePath = outDir.filePath(imageName);
        if (image.save(imagePath, "PNG")) {
            slideImagePaths.append(imagePath);
        }
    }

    if (slideImagePaths.isEmpty()) {
        errorMessage = QStringLiteral("Failed to render slide images from PDF.");
        return false;
    }

    return true;
#else
    QProcess process;
    process.setProgram(libreOfficeExecutable);

    QStringList args;
    args << QStringLiteral("--headless")
         << QStringLiteral("--nologo")
         << QStringLiteral("--norestore")
         << QStringLiteral("--convert-to") << QStringLiteral("png")
         << QStringLiteral("--outdir") << outputDir
         << filePath;
    process.setArguments(args);
    process.setProcessChannelMode(QProcess::MergedChannels);

    process.start();
    if (!process.waitForStarted(30000)) {
        errorMessage = QStringLiteral("Failed to start LibreOffice process.");
        return false;
    }

    if (!process.waitForFinished(120000)) {
        process.kill();
        process.waitForFinished(5000);
        errorMessage = QStringLiteral("LibreOffice did not finish conversion in time.");
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QString output = QString::fromLocal8Bit(process.readAll());
        if (output.isEmpty()) {
            output = QStringLiteral("Unknown error from LibreOffice.");
        }
        errorMessage = QStringLiteral("LibreOffice conversion failed: %1").arg(output.trimmed());
        return false;
    }

    QDir outDir(outputDir);
    QStringList pngFiles = outDir.entryList(QStringList() << QStringLiteral("*.png") << QStringLiteral("*.PNG"),
                                            QDir::Files,
                                            QDir::Name);
    if (pngFiles.isEmpty()) {
        errorMessage = QStringLiteral("LibreOffice reported success but no slide images were generated.");
        return false;
    }

    slideImagePaths.clear();
    for (const QString &name : pngFiles) {
        slideImagePaths.append(outDir.filePath(name));
    }

    return true;
#endif
#else
    Q_UNUSED(filePath);
    Q_UNUSED(outputDir);
    Q_UNUSED(errorMessage);
    return false;
#endif
}
