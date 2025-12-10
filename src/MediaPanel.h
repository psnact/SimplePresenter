#ifndef MEDIAPANEL_H
#define MEDIAPANEL_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QListWidgetItem>
#include <QPoint>
#include <QMenu>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoSink>
#include <QVideoFrame>

class QStackedWidget;

class MediaPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MediaPanel(QWidget *parent = nullptr);
    ~MediaPanel();

signals:
    void mediaSelected(const QString &path, bool isVideo);
    void mediaAddedToPlaylist(const QString &displayName, const QString &path, bool isVideo);
    void mediaAboutToRemove(const QString &path, bool wasVideo);
    void mediaRemoved(const QString &path, bool wasVideo);

public slots:
    void onTabChanged();

private slots:
    void onAddImage();
    void onAddVideo();
    void onRemoveMedia();
    void onAddToPlaylist();
    void onMediaItemClicked(QListWidgetItem *item);
    void onMediaDoubleClicked(QListWidgetItem *item);
    void onCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void onSelectionChanged();
    void onContextMenuRequested(const QPoint &pos);
    void onPreviewMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPreviewVideoFrameChanged(const QVideoFrame &frame);

private:
    void setupUI();
    void loadMediaLibrary();
    void addMediaItem(const QString &filePath, bool isVideo, bool copyFile = true);
    QString mediaLibraryPath() const;
    void updatePreviewForItem(QListWidgetItem *item);
    void updateVideoLabelPixmap();

    QListWidget *mediaList;
    QPushButton *addImageButton;
    QPushButton *addVideoButton;
    QPushButton *removeButton;
    QPushButton *addToPlaylistButton;
    QStackedWidget *previewStack;
    QLabel *previewLabel;
    QLabel *previewVideoLabel;
    QMediaPlayer *previewPlayer;
    QAudioOutput *previewAudioOutput;
    QVideoSink *previewVideoSink;
    QString currentPreviewPath;
    QTimer *previewStartTimer;
    bool previewAutoStartPending;
    QPixmap previewVideoPixmap;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // MEDIAPANEL_H
