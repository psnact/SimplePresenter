#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QSettings>
#include <QMediaPlayer>
#include <QVector>

class BiblePanel;
class SongPanel;
class PlaylistPanel;
class ProjectionCanvas;
class MediaPanel;
class PowerPointPanel;
class OverlayServer;
class UpdateChecker;
class QToolButton;
class QSlider;
class QTimer;
class QVBoxLayout;
class QLabel;
class QLineEdit;
class QTextEdit;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsProxyWidget;
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
class QWebEngineView;
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void newPlaylist();
    void openPlaylist();
    void savePlaylist();
    void savePlaylistAs();
    void showSettings();
    void showAbout();
    void clearOverlays();
    void projectBibleVerse(const QString &reference, const QString &text);
    void projectSongSection(const QString &songTitle, const QString &sectionText);
    void projectPlaylistItem(int index);
    void projectMediaItem(const QString &path, bool isVideo);
    void onMediaVideoStarted();
    void onMediaVideoStopped();
    void onMediaPlayPauseClicked();
    void onMediaPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaPositionChanged(qint64 position);
    void onMediaDurationChanged(qint64 duration);
    void onMediaSeekSliderPressed();
    void onMediaSeekSliderReleased();
    void onMediaSeekSliderMoved(int value);
    void onMediaVolumeChanged(int value);
    void onMediaEntryAboutToRemove(const QString &path, bool wasVideo);
    void onMediaEntryRemoved(const QString &path, bool wasVideo);
    void onNotesTextChanged();
    void onNotesPrevPage();
    void onNotesNextPage();
    void onNotesAddPage();
    void toggleExternalProjection(bool checked);
    void onCheckForUpdates();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupMediaControls(QVBoxLayout *projectionLayout);
    void updateMediaPlayPauseIcon();
    void loadSettings();
    void saveSettings();
    void loadServiceNotes(const QString &filePath);
    void saveServiceNotes(const QString &filePath);
    void updateProjectionAndNotesAspect();
    void updateNotesOverlayAsMedia();
    void syncYouTubeOverlayToProjection();
    void syncMediaOverlayToProjection();

    // UI Components
    QSplitter *mainSplitter;
    QSplitter *verticalSplitter;
    QTabWidget *contentTabs;
    BiblePanel *biblePanel;
    SongPanel *songPanel;
    MediaPanel *mediaPanel;
    PowerPointPanel *powerPointPanel;
    PlaylistPanel *playlistPanel;
    
    // Canvas previews (embedded in main window)
    ProjectionCanvas *projectionCanvas;
    
    // Fullscreen projection window (separate window for external display)
    ProjectionCanvas *fullscreenProjection;

    // Projection notes editor shown next to the preview canvas
    QTabWidget *notesTabWidget;
    QTextEdit *notesEditor;
    QGraphicsView *notesPreviewView;
    QGraphicsScene *notesScene;
    QGraphicsProxyWidget *notesProxy;
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    QWebEngineView *youtubeBrowser;
#endif
    QToolButton *notesShowButton;
    QToolButton *notesPrevPageButton;
    QToolButton *notesNextPageButton;
    QToolButton *notesAddPageButton;
    QLineEdit *notesPageJumpEdit;
    QLabel *notesPageStatusLabel;
    QVector<QString> notesPages;
    int currentNotesPageIndex;
    QString notesLastGoodHtml;
    bool notesPageFull;
    
    // Overlay server for OBS
    OverlayServer *overlayServer;

    // Update Checker
    UpdateChecker *updateChecker;

    // Media playback controls
    QWidget *mediaControlsWidget;
    QToolButton *mediaPlayPauseButton;
    QSlider *mediaSeekSlider;
    QSlider *mediaVolumeSlider;
    bool mediaSeekSliderDragging;
    bool youtubePlaying;
    QTimer *youtubePositionTimer;

    // Toolbar actions
    QAction *newAction;
    QAction *openAction;
    QAction *saveAction;
    QAction *settingsAction;
    QAction *clearAction;
    QAction *toggleProjectionDisplayAction;
    
    // Settings
    QSettings *settings;
    QString currentPlaylistFile;
};

#endif // MAINWINDOW_H
