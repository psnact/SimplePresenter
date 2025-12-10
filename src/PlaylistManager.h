#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>

enum class PlaylistItemType {
    BibleVerse,
    Song,
    Media,
    YouTube
};

struct PlaylistItem {
    PlaylistItemType type = PlaylistItemType::BibleVerse;
    QString title;        // Display title
    QString reference;    // Bible reference, song title, or description
    QJsonObject data;     // Additional data (verses, sections, media info, etc.)
    
    QString displayText() const {
        return title.isEmpty() ? reference : title;
    }
};

class PlaylistManager : public QObject
{
    Q_OBJECT

public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager();
    
    // Playlist operations
    void clear();
    void addItem(const PlaylistItem &item);
    void removeItem(int index);
    void moveItem(int fromIndex, int toIndex);
    PlaylistItem getItem(int index) const;
    int itemCount() const { return items.size(); }
    
    // File operations
    bool loadPlaylist(const QString &filePath);
    bool savePlaylist(const QString &filePath);
    
    // State
    bool hasUnsavedChanges() const { return unsavedChanges; }
    void markSaved() { unsavedChanges = false; }
    
    // Get all items
    const QVector<PlaylistItem>& getItems() const { return items; }

signals:
    void playlistChanged();
    void itemAdded(int index);
    void itemRemoved(int index);
    void itemMoved(int fromIndex, int toIndex);

private:
    QVector<PlaylistItem> items;
    bool unsavedChanges;
};

#endif // PLAYLISTMANAGER_H
