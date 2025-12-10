#include "SongManager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QXmlStreamReader>

SongManager::SongManager(QObject *parent)
    : QObject(parent)
{
}
SongManager::~SongManager()
{
}

void SongManager::loadSongsFromDirectory(const QString &dirPath)
{
    songs.clear();
    
    QDir dir(dirPath);
    if (!dir.exists()) {
        emit songLoadError(QString("Directory does not exist: %1").arg(dirPath));
        return;
    }
    
    QStringList filters;
    filters << "*.xml" << "*.txt";  // Support both XML and TXT files
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    int loadedCount = 0;
    
    for (const QFileInfo &fileInfo : files) {
        if (loadSong(fileInfo.absoluteFilePath())) {
            loadedCount++;
        }
    }
    
    emit songsLoaded(loadedCount);
}

bool SongManager::loadSong(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit songLoadError(QString("Cannot open file: %1").arg(filePath));
        return false;
    }
    
    Song song;
    song.filePath = filePath;
    
    QXmlStreamReader xml(&file);
    
    SongSection currentSection;
    int sectionIndex = 0;
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            QString name = xml.name().toString();
            
            if (name == "title") {
                song.title = xml.readElementText();
            } else if (name == "author") {
                song.author = xml.readElementText();
            } else if (name == "copyright") {
                song.copyright = xml.readElementText();
            } else if (name == "ccli") {
                song.ccli = xml.readElementText();
            } else if (name == "section") {
                currentSection = SongSection();
                currentSection.index = sectionIndex++;
                currentSection.type = xml.attributes().value("type").toString();
                if (currentSection.type.isEmpty()) {
                    currentSection.type = "verse";
                }
            } else if (name == "line") {
                currentSection.lines.append(xml.readElementText());
            }
        } else if (xml.isEndElement() && xml.name() == "section") {
            if (!currentSection.lines.isEmpty()) {
                song.sections.append(currentSection);
            }
        }
    }
    
    file.close();
    
    if (xml.hasError()) {
        emit songLoadError(QString("XML error in %1: %2").arg(filePath).arg(xml.errorString()));
        return false;
    }
    
    if (song.title.isEmpty()) {
        song.title = QFileInfo(filePath).baseName();
    }
    
    if (song.sections.isEmpty()) {
        emit songLoadError(QString("No content found in: %1").arg(filePath));
        return false;
    }
    
    songs[song.title] = song;
    return true;
}

QStringList SongManager::getSongTitles() const
{
    QStringList titles = songs.keys();
    titles.sort(Qt::CaseInsensitive);
    return titles;
}

Song SongManager::getSong(const QString &title) const
{
    if (songs.contains(title)) {
        return songs[title];
    }
    return Song();
}

QStringList SongManager::searchSongs(const QString &searchText) const
{
    QStringList results;
    QString lowerSearch = searchText.toLower();
    
    for (auto it = songs.constBegin(); it != songs.constEnd(); ++it) {
        if (it.key().toLower().contains(lowerSearch)) {
            results.append(it.key());
        }
    }
    
    results.sort(Qt::CaseInsensitive);
    return results;
}

QString SongManager::getSongFilePath(const QString &title) const
{
    if (songs.contains(title)) {
        return songs[title].filePath;
    }
    return QString();
}
