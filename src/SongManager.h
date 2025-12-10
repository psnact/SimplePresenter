#ifndef SONGMANAGER_H
#define SONGMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>

struct SongSection {
    int index;
    QString type;  // verse, chorus, bridge, etc.
    QStringList lines;
    
    QString text() const {
        return lines.join("\n");
    }
};

struct Song {
    QString title;
    QString author;
    QString copyright;
    QString ccli;
    QString filePath;
    QVector<SongSection> sections;
};

class SongManager : public QObject
{
    Q_OBJECT

public:
    explicit SongManager(QObject *parent = nullptr);
    ~SongManager();
    
    // Load all songs from directory
    void loadSongsFromDirectory(const QString &dirPath = "data/songs");
    
    // Load single song
    bool loadSong(const QString &filePath);
    
    // Get list of song titles
    QStringList getSongTitles() const;
    
    // Get song by title
    Song getSong(const QString &title) const;
    
    // Search songs by title
    QStringList searchSongs(const QString &searchText) const;
    
    // Get song file path
    QString getSongFilePath(const QString &title) const;

signals:
    void songsLoaded(int count);
    void songLoadError(const QString &error);

private:
    QMap<QString, Song> songs; // title -> song
};

#endif // SONGMANAGER_H
