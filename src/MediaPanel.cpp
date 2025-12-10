#include "MediaPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMessageBox>
#include <QPixmap>
#include <QImage>
#include <QFile>
#include <QIcon>
#include <QStyle>
#include <QMenu>
#include <QUrl>
#include <QItemSelectionModel>
#include <QTimer>
#include <QEvent>

MediaPanel::MediaPanel(QWidget *parent)
    : QWidget(parent)
    , previewStartTimer(new QTimer(this))
    , previewAutoStartPending(false)
{
    setupUI();
    loadMediaLibrary();

    // Timer no longer needed - direct play works better
}

MediaPanel::~MediaPanel()
{
}

void MediaPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *description = new QLabel("Media Library");
    description->setWordWrap(true);
    mainLayout->addWidget(description);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    mainLayout->addLayout(contentLayout, 1);

    QVBoxLayout *libraryLayout = new QVBoxLayout();
    contentLayout->addLayout(libraryLayout, 2);

    mediaList = new QListWidget(this);
    mediaList->setSelectionMode(QAbstractItemView::SingleSelection);
    mediaList->setDragEnabled(true);
    mediaList->setViewMode(QListView::ListMode);
    mediaList->setSpacing(4);
    mediaList->setContextMenuPolicy(Qt::CustomContextMenu);
    libraryLayout->addWidget(mediaList, 1);

    connect(mediaList, &QListWidget::itemClicked,
            this, &MediaPanel::onMediaItemClicked);
    connect(mediaList, &QListWidget::itemDoubleClicked,
            this, &MediaPanel::onMediaDoubleClicked);
    connect(mediaList, &QListWidget::currentItemChanged,
            this, &MediaPanel::onCurrentItemChanged);
    connect(mediaList, &QListWidget::itemSelectionChanged,
            this, &MediaPanel::onSelectionChanged);
    connect(mediaList, &QListWidget::customContextMenuRequested,
            this, &MediaPanel::onContextMenuRequested);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addImageButton = new QPushButton("Add Image", this);
    addVideoButton = new QPushButton("Add Video", this);
    removeButton = new QPushButton("Remove", this);
    addToPlaylistButton = new QPushButton("Add to Playlist", this);

    buttonLayout->addWidget(addImageButton);
    buttonLayout->addWidget(addVideoButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(addToPlaylistButton);
    libraryLayout->addLayout(buttonLayout);

    connect(addImageButton, &QPushButton::clicked,
            this, &MediaPanel::onAddImage);
    connect(addVideoButton, &QPushButton::clicked,
            this, &MediaPanel::onAddVideo);
    connect(removeButton, &QPushButton::clicked,
            this, &MediaPanel::onRemoveMedia);
    connect(addToPlaylistButton, &QPushButton::clicked,
            this, &MediaPanel::onAddToPlaylist);

    QVBoxLayout *previewLayout = new QVBoxLayout();
    contentLayout->addLayout(previewLayout, 3);

    previewStack = new QStackedWidget(this);
    previewStack->setMinimumSize(320, 240);
    previewStack->setStyleSheet("border: 1px solid #666; background-color: #111;");

    previewLabel = new QLabel(previewStack);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setWordWrap(true);
    previewLabel->setText("Select an image to preview");
    previewStack->addWidget(previewLabel);

    previewVideoLabel = new QLabel(previewStack);
    previewVideoLabel->setAlignment(Qt::AlignCenter);
    previewVideoLabel->setStyleSheet("background-color: black;");
    previewVideoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewVideoLabel->setMinimumSize(320, 240);
    previewStack->addWidget(previewVideoLabel);
    previewVideoLabel->installEventFilter(this);

    previewLayout->addWidget(previewStack, 1);

    previewPlayer = new QMediaPlayer(this);
    previewAudioOutput = new QAudioOutput(this);
    previewAudioOutput->setMuted(true);
    previewAudioOutput->setVolume(0.0);
    previewPlayer->setAudioOutput(previewAudioOutput);

    previewVideoSink = new QVideoSink(this);
    connect(previewVideoSink, &QVideoSink::videoFrameChanged,
            this, &MediaPanel::onPreviewVideoFrameChanged);
    previewPlayer->setVideoSink(previewVideoSink);

    connect(previewPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &MediaPanel::onPreviewMediaStatusChanged);

    previewPlayer->setLoops(QMediaPlayer::Infinite);

    onSelectionChanged();
}

QString MediaPanel::mediaLibraryPath() const
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    dir.mkpath("media");
    dir.cd("media");
    return dir.absolutePath();
}

void MediaPanel::onContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = mediaList->itemAt(pos);
    QMenu menu(this);

    QAction *addImageAction = menu.addAction("Add Image...");
    QAction *addVideoAction = menu.addAction("Add Video...");
    QAction *removeAction = nullptr;
    QAction *addToPlaylistAction = nullptr;

    if (item) {
        menu.addSeparator();
        removeAction = menu.addAction("Remove from Library");
        addToPlaylistAction = menu.addAction("Add to Playlist");
    }

    QAction *chosen = menu.exec(mediaList->viewport()->mapToGlobal(pos));
    if (!chosen) {
        return;
    }

    if (chosen == addImageAction) {
        onAddImage();
    } else if (chosen == addVideoAction) {
        onAddVideo();
    } else if (removeAction && chosen == removeAction) {
        mediaList->setCurrentItem(item);
        onRemoveMedia();
    } else if (addToPlaylistAction && chosen == addToPlaylistAction) {
        mediaList->setCurrentItem(item);
        onAddToPlaylist();
    }
}

void MediaPanel::loadMediaLibrary()
{
    mediaList->clear();

    QDir dir(mediaLibraryPath());
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.gif"
            << "*.mp4" << "*.mov" << "*.avi" << "*.mkv";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const QFileInfo &info : files) {
        const QString suffix = info.suffix().toLower();
        bool isVideo = suffix == "mp4" || suffix == "mov" || suffix == "avi" || suffix == "mkv" || suffix == "webm";
        addMediaItem(info.absoluteFilePath(), isVideo, /*copyFile=*/false);
    }
}

void MediaPanel::addMediaItem(const QString &filePath, bool isVideo, bool copyFile)
{
    QFileInfo originalInfo(filePath);
    QString destinationPath = filePath;

    if (copyFile) {
        QDir library(mediaLibraryPath());
        QString baseName = originalInfo.completeBaseName();
        QString suffix = originalInfo.suffix();
        QString newFileName = originalInfo.fileName();
        destinationPath = library.filePath(newFileName);

        int counter = 1;
        while (QFile::exists(destinationPath)) {
            newFileName = QString("%1_%2.%3").arg(baseName).arg(counter++).arg(suffix);
            destinationPath = library.filePath(newFileName);
        }

        if (!QFile::copy(filePath, destinationPath)) {
            QMessageBox::warning(this, "Copy Failed",
                                 QString("Could not copy %1 to media library.").arg(originalInfo.fileName()));
            return;
        }
    }

    QFileInfo storedInfo(destinationPath);

    QString displayName = QString("%1.%2").arg(storedInfo.completeBaseName(), storedInfo.suffix());
    QListWidgetItem *item = new QListWidgetItem(displayName, mediaList);
    item->setData(Qt::UserRole, destinationPath);
    item->setData(Qt::UserRole + 1, isVideo);
    item->setToolTip(destinationPath);

    QIcon defaultIcon = style()->standardIcon(QStyle::SP_FileIcon);
    if (!isVideo) {
        QPixmap pix(destinationPath);
        if (!pix.isNull()) {
            item->setIcon(QIcon(pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        } else {
            item->setIcon(defaultIcon);
        }
    } else {
        QIcon videoIcon = QIcon::fromTheme("video-x-generic", defaultIcon);
        item->setIcon(videoIcon);
    }
}

void MediaPanel::onAddImage()
{
#ifdef Q_OS_MACOS
    QFileDialog dialog(this, "Select Images");
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    dialog.setNameFilter("Images (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() == QDialog::Accepted) {
        const QStringList files = dialog.selectedFiles();
        for (const QString &file : files) {
            addMediaItem(file, /*isVideo=*/false, /*copyFile=*/true);
        }
    }
#else
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Select Images",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Images (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*)");

    for (const QString &file : files) {
        addMediaItem(file, /*isVideo=*/false, /*copyFile=*/true);
    }
#endif
}

void MediaPanel::onAddVideo()
{
#ifdef Q_OS_MACOS
    QFileDialog dialog(this, "Select Videos");
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
    dialog.setNameFilter("Videos (*.mp4 *.mov *.avi *.mkv *.webm);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() == QDialog::Accepted) {
        const QStringList files = dialog.selectedFiles();
        for (const QString &file : files) {
            addMediaItem(file, /*isVideo=*/true, /*copyFile=*/true);
        }
    }
#else
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Select Videos",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        "Videos (*.mp4 *.mov *.avi *.mkv *.webm);;All Files (*)");

    for (const QString &file : files) {
        addMediaItem(file, /*isVideo=*/true, /*copyFile=*/true);
    }
#endif
}

void MediaPanel::onRemoveMedia()
{
    QListWidgetItem *item = mediaList->currentItem();
    if (!item) {
        return;
    }

    QString path = item->data(Qt::UserRole).toString();
    bool isVideo = item->data(Qt::UserRole + 1).toBool();

    emit mediaAboutToRemove(path, isVideo);

    if (previewPlayer) {
        previewPlayer->stop();
        previewPlayer->setSource(QUrl());
    }
    previewStack->setCurrentWidget(previewLabel);
    previewLabel->setText("Select an image to preview");
    previewLabel->setPixmap(QPixmap());

    QFile file(path);
    if (file.exists() && !file.remove()) {
        QMessageBox::warning(this, "Remove Failed",
                             QString("Could not remove %1 from disk.").arg(QFileInfo(path).fileName()));
    }

    delete item;

    if (mediaList->count() == 0) {
        removeButton->setEnabled(false);
        addToPlaylistButton->setEnabled(false);
    }

    emit mediaRemoved(path, isVideo);
}

void MediaPanel::onMediaItemClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }

    mediaList->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
    updatePreviewForItem(item);
}

void MediaPanel::updatePreviewForItem(QListWidgetItem *item)
{
    if (!item) {
        return;
    }

    QString path = item->data(Qt::UserRole).toString();
    bool isVideo = item->data(Qt::UserRole + 1).toBool();

    removeButton->setEnabled(true);
    addToPlaylistButton->setEnabled(true);

    if (!isVideo) {
        previewPlayer->stop();
        QPixmap pix(path);
        if (!pix.isNull()) {
            previewLabel->setPixmap(pix.scaled(previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            previewStack->setCurrentWidget(previewLabel);
        } else {
            previewLabel->setText("(Preview unavailable)");
            previewLabel->setPixmap(QPixmap());
            previewStack->setCurrentWidget(previewLabel);
        }
    } else {
        // Show video widget first
        previewStack->setCurrentWidget(previewVideoLabel);
        previewVideoLabel->show();

        const bool newSource = (currentPreviewPath != path);
        if (newSource) {
            previewAutoStartPending = true;
            previewPlayer->stop();
            previewPlayer->setSource(QUrl::fromLocalFile(path));
            currentPreviewPath = path;
        }

        if (newSource || previewPlayer->playbackState() != QMediaPlayer::PlayingState) {
            previewPlayer->setPosition(0);
            previewAutoStartPending = true;
            QMetaObject::invokeMethod(previewPlayer, [this]() {
                previewPlayer->play();
            }, Qt::QueuedConnection);
        }
    }
}

void MediaPanel::onPreviewMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (!previewAutoStartPending) {
        return;
    }

    if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
        previewAutoStartPending = false;
        previewPlayer->play();
    } else if (status == QMediaPlayer::InvalidMedia || status == QMediaPlayer::NoMedia) {
        previewAutoStartPending = false;
    }
}

void MediaPanel::onPreviewVideoFrameChanged(const QVideoFrame &frame)
{
    if (!frame.isValid()) {
        return;
    }

    const QImage image = frame.toImage();
    if (image.isNull()) {
        return;
    }

    previewVideoPixmap = QPixmap::fromImage(image);
    updateVideoLabelPixmap();
}

void MediaPanel::updateVideoLabelPixmap()
{
    if (previewVideoPixmap.isNull()) {
        return;
    }

    int width = previewVideoLabel->width();
    int height = previewVideoLabel->height();
    int pixmapWidth = previewVideoPixmap.width();
    int pixmapHeight = previewVideoPixmap.height();

    double aspectRatio = static_cast<double>(pixmapWidth) / pixmapHeight;
    int newWidth = width;
    int newHeight = static_cast<int>(width / aspectRatio);

    if (newHeight > height) {
        newHeight = height;
        newWidth = static_cast<int>(height * aspectRatio);
    }

    QPixmap scaledPixmap = previewVideoPixmap.scaled(newWidth, newHeight, Qt::KeepAspectRatio);
    previewVideoLabel->setPixmap(scaledPixmap);
}

void MediaPanel::onAddToPlaylist()
{
    QListWidgetItem *item = mediaList->currentItem();
    if (!item) {
        return;
    }

    QString path = item->data(Qt::UserRole).toString();
    bool isVideo = item->data(Qt::UserRole + 1).toBool();
    emit mediaAddedToPlaylist(item->text(), path, isVideo);
}

void MediaPanel::onMediaDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    bool isVideo = item->data(Qt::UserRole + 1).toBool();
    emit mediaSelected(path, isVideo);
}

void MediaPanel::onCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);

    if (current) {
        updatePreviewForItem(current);
    }
}

void MediaPanel::onSelectionChanged()
{
    QListWidgetItem *item = mediaList->currentItem();
    bool hasSelection = item != nullptr;
    removeButton->setEnabled(hasSelection);
    addToPlaylistButton->setEnabled(hasSelection);

    if (!hasSelection) {
        previewPlayer->stop();
        previewStack->setCurrentWidget(previewLabel);
        previewLabel->setText("Select an image to preview");
        previewLabel->setPixmap(QPixmap());
        return;
    }

    updatePreviewForItem(item);
}

void MediaPanel::onTabChanged()
{
    if (previewPlayer) {
        previewPlayer->stop();
    }
    previewStack->setCurrentWidget(previewLabel);
    previewLabel->setText("Select an image to preview");
    previewLabel->setPixmap(QPixmap());
    currentPreviewPath.clear();
    previewVideoPixmap = QPixmap();
}

bool MediaPanel::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == previewVideoLabel && event->type() == QEvent::Resize) {
        updateVideoLabelPixmap();
    }
    return QWidget::eventFilter(watched, event);
}
