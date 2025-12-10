#ifndef PLAYLISTPANEL_H
#define PLAYLISTPANEL_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include "PlaylistManager.h"

class PlaylistPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistPanel(QWidget *parent = nullptr);
    ~PlaylistPanel();
    
    // Playlist operations
    void clear();
    bool loadPlaylist(const QString &filePath);
    bool savePlaylist(const QString &filePath);
    bool hasUnsavedChanges() const;
    
    // Add items to playlist
    void addBibleVerse(const QString &reference);
    void addSong(const QString &songTitle, const QString &sectionText = QString());
    void addMedia(const QString &displayName, const QString &path, bool isVideo);
    void addYouTube(const QString &url, const QString &title = QString());

signals:
    void itemActivated(int index);
    void bibleVerseActivated(const QString &reference);
    void songActivated(const QString &songTitle);
    void mediaActivated(const QString &path, bool isVideo);
    void youtubeActivated(const QString &url);

private slots:
    void onItemDoubleClicked(QListWidgetItem *item);
    void onRemoveClicked();
    void onMoveUpClicked();
    void onMoveDownClicked();
    void onPlaylistChanged();

private:
    enum class FilterMode {
        All,
        Bible,
        Songs,
        MediaAll,
        MediaVideos,
        MediaImages,
        YouTube
    };

    void setupUI();
    void refreshList();
    bool matchesFilter(const PlaylistItem &item) const;
    int rowForItemIndex(int itemIndex) const;
    void flashRow(int row);
    void highlightExistingItem(int itemIndex);
    
    PlaylistManager *playlistManager;
    
    QListWidget *playlistList;
    QPushButton *removeButton;
    QPushButton *moveUpButton;
    QPushButton *moveDownButton;
    QComboBox *filterCombo;
    FilterMode currentFilter;
};

#endif // PLAYLISTPANEL_H
