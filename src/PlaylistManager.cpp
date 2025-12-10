#include "PlaylistManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

PlaylistManager::PlaylistManager(QObject *parent)
    : QObject(parent)
    , unsavedChanges(false)
{
}

PlaylistManager::~PlaylistManager()
{
}

void PlaylistManager::clear()
{
    items.clear();
    unsavedChanges = true;
    emit playlistChanged();
}

void PlaylistManager::addItem(const PlaylistItem &item)
{
    items.append(item);
    unsavedChanges = true;
    emit itemAdded(items.size() - 1);
    emit playlistChanged();
}

void PlaylistManager::removeItem(int index)
{
    if (index >= 0 && index < items.size()) {
        items.remove(index);
        unsavedChanges = true;
        emit itemRemoved(index);
        emit playlistChanged();
    }
}

void PlaylistManager::moveItem(int fromIndex, int toIndex)
{
    if (fromIndex >= 0 && fromIndex < items.size() &&
        toIndex >= 0 && toIndex < items.size() &&
        fromIndex != toIndex) {
        
        PlaylistItem item = items[fromIndex];
        items.remove(fromIndex);
        items.insert(toIndex, item);
        
        unsavedChanges = true;
        emit itemMoved(fromIndex, toIndex);
        emit playlistChanged();
    }
}

PlaylistItem PlaylistManager::getItem(int index) const
{
    if (index >= 0 && index < items.size()) {
        return items[index];
    }
    return PlaylistItem();
}

bool PlaylistManager::loadPlaylist(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray itemsArray = root["items"].toArray();
    
    items.clear();
    
    for (const QJsonValue &value : itemsArray) {
        QJsonObject itemObj = value.toObject();
        
        PlaylistItem item;
        
        QString typeStr = itemObj["type"].toString();
        if (typeStr == "bible") {
            item.type = PlaylistItemType::BibleVerse;
        } else if (typeStr == "song") {
            item.type = PlaylistItemType::Song;
        } else if (typeStr == "media") {
            item.type = PlaylistItemType::Media;
        } else if (typeStr == "youtube") {
            item.type = PlaylistItemType::YouTube;
        } else {
            continue;
        }
        
        item.title = itemObj["title"].toString();
        item.reference = itemObj["reference"].toString();
        item.data = itemObj["data"].toObject();
        
        items.append(item);
    }
    
    unsavedChanges = false;
    emit playlistChanged();
    
    return true;
}

bool PlaylistManager::savePlaylist(const QString &filePath)
{
    QJsonObject root;
    root["version"] = "1.0";
    
    QJsonArray itemsArray;
    
    for (const PlaylistItem &item : items) {
        QJsonObject itemObj;
        
        if (item.type == PlaylistItemType::BibleVerse) {
            itemObj["type"] = "bible";
        } else if (item.type == PlaylistItemType::Song) {
            itemObj["type"] = "song";
        } else if (item.type == PlaylistItemType::Media) {
            itemObj["type"] = "media";
        } else if (item.type == PlaylistItemType::YouTube) {
            itemObj["type"] = "youtube";
        }
        
        itemObj["title"] = item.title;
        itemObj["reference"] = item.reference;
        itemObj["data"] = item.data;
        
        itemsArray.append(itemObj);
    }
    
    root["items"] = itemsArray;
    
    QJsonDocument doc(root);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    unsavedChanges = false;
    
    return true;
}
