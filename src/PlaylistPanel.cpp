#include "PlaylistPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QStyle>
#include <QTimer>

PlaylistPanel::PlaylistPanel(QWidget *parent)
    : QWidget(parent)
    , playlistManager(new PlaylistManager(this))
    , filterCombo(nullptr)
    , currentFilter(FilterMode::All)
{
    setupUI();
    
    connect(playlistManager, &PlaylistManager::playlistChanged,
            this, &PlaylistPanel::onPlaylistChanged);
}

PlaylistPanel::~PlaylistPanel()
{
}

void PlaylistPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QLabel *titleLabel = new QLabel("Playlist / Order of Service");
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    QHBoxLayout *filterLayout = new QHBoxLayout();
    QLabel *filterLabel = new QLabel("Filter:");
    filterCombo = new QComboBox(this);
    filterCombo->addItem("All");
    filterCombo->addItem("Bible");
    filterCombo->addItem("Songs");
    filterCombo->addItem("Media");
    filterCombo->addItem("Videos");
    filterCombo->addItem("Photos");
    filterCombo->addItem("YouTube");
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(filterCombo);
    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);
    
    // Playlist list
    playlistList = new QListWidget();
    playlistList->setDragDropMode(QAbstractItemView::InternalMove);  // Allow rearranging within playlist
    playlistList->setAcceptDrops(true);
    playlistList->setContextMenuPolicy(Qt::CustomContextMenu);
    mainLayout->addWidget(playlistList);
    
    // Control buttons (removed Add Bible/Song buttons)
    QHBoxLayout *controlLayout = new QHBoxLayout();
    removeButton = new QPushButton("Remove");
    moveUpButton = new QPushButton("Move Up");
    moveDownButton = new QPushButton("Move Down");
    
    removeButton->setEnabled(false);
    moveUpButton->setEnabled(false);
    moveDownButton->setEnabled(false);
    
    controlLayout->addWidget(removeButton);
    controlLayout->addWidget(moveUpButton);
    controlLayout->addWidget(moveDownButton);
    mainLayout->addLayout(controlLayout);
    
    // Connect signals
    connect(playlistList, &QListWidget::itemDoubleClicked,
            this, &PlaylistPanel::onItemDoubleClicked);
    connect(playlistList, &QListWidget::itemSelectionChanged, [this]() {
        bool hasSelection = playlistList->currentRow() >= 0;
        removeButton->setEnabled(hasSelection);
        moveUpButton->setEnabled(hasSelection && playlistList->currentRow() > 0);
        moveDownButton->setEnabled(hasSelection && playlistList->currentRow() < playlistList->count() - 1);
    });
    
    connect(removeButton, &QPushButton::clicked,
            this, &PlaylistPanel::onRemoveClicked);
    connect(moveUpButton, &QPushButton::clicked,
            this, &PlaylistPanel::onMoveUpClicked);
    connect(moveDownButton, &QPushButton::clicked,
            this, &PlaylistPanel::onMoveDownClicked);

    if (filterCombo) {
        connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int index) {
                    switch (index) {
                    case 0: currentFilter = FilterMode::All; break;
                    case 1: currentFilter = FilterMode::Bible; break;
                    case 2: currentFilter = FilterMode::Songs; break;
                    case 3: currentFilter = FilterMode::MediaAll; break;
                    case 4: currentFilter = FilterMode::MediaVideos; break;
                    case 5: currentFilter = FilterMode::MediaImages; break;
                    case 6: currentFilter = FilterMode::YouTube; break;
                    default: currentFilter = FilterMode::All; break;
                    }
                    refreshList();
                });
    }
}

void PlaylistPanel::clear()
{
    playlistManager->clear();
    refreshList();
}

bool PlaylistPanel::loadPlaylist(const QString &filePath)
{
    if (playlistManager->loadPlaylist(filePath)) {
        refreshList();
        return true;
    }
    return false;
}

bool PlaylistPanel::savePlaylist(const QString &filePath)
{
    return playlistManager->savePlaylist(filePath);
}

bool PlaylistPanel::hasUnsavedChanges() const
{
    return playlistManager->hasUnsavedChanges();
}

void PlaylistPanel::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    int index = playlistList->row(item);
    emit itemActivated(index);
    
    // Emit specific signals based on item type
    PlaylistItem playlistItem = playlistManager->getItem(index);
    
    if (playlistItem.type == PlaylistItemType::BibleVerse) {
        emit bibleVerseActivated(playlistItem.reference);
    } else if (playlistItem.type == PlaylistItemType::Song) {
        emit songActivated(playlistItem.title);
    } else if (playlistItem.type == PlaylistItemType::Media) {
        const QString path = playlistItem.data.value("path").toString(playlistItem.reference);
        bool isVideo = playlistItem.data.value("isVideo").toBool();
        emit mediaActivated(path, isVideo);
    } else if (playlistItem.type == PlaylistItemType::YouTube) {
        const QString url = playlistItem.data.value("url").toString(playlistItem.reference);
        emit youtubeActivated(url);
    }
}

void PlaylistPanel::addBibleVerse(const QString &reference)
{
    const QVector<PlaylistItem> &items = playlistManager->getItems();
    for (int i = 0; i < items.size(); ++i) {
        const PlaylistItem &existing = items[i];
        if (existing.type == PlaylistItemType::BibleVerse && existing.reference == reference) {
            highlightExistingItem(i);
            return;
        }
    }

    PlaylistItem item;
    item.type = PlaylistItemType::BibleVerse;
    item.title = reference;
    item.reference = reference;
    playlistManager->addItem(item);
}

void PlaylistPanel::addSong(const QString &songTitle, const QString &sectionText)
{
    const QString sectionKey = sectionText.isEmpty() ? songTitle : sectionText;
    const QVector<PlaylistItem> &items = playlistManager->getItems();
    for (int i = 0; i < items.size(); ++i) {
        const PlaylistItem &existing = items[i];
        if (existing.type == PlaylistItemType::Song &&
            existing.title == songTitle &&
            existing.reference == sectionKey) {
            highlightExistingItem(i);
            return;
        }
    }

    PlaylistItem item;
    item.type = PlaylistItemType::Song;
    item.title = songTitle;
    item.reference = sectionKey;
    playlistManager->addItem(item);
}

void PlaylistPanel::addMedia(const QString &displayName, const QString &path, bool isVideo)
{
    const QVector<PlaylistItem> &items = playlistManager->getItems();
    for (int i = 0; i < items.size(); ++i) {
        const PlaylistItem &existing = items[i];
        if (existing.type == PlaylistItemType::Media) {
            const QString existingPath = existing.data.value("path").toString(existing.reference);
            const bool existingIsVideo = existing.data.value("isVideo").toBool();
            if (existingPath == path && existingIsVideo == isVideo) {
                highlightExistingItem(i);
                return;
            }
        }
    }

    PlaylistItem item;
    item.type = PlaylistItemType::Media;
    item.title = displayName;
    item.reference = path;
    item.data["path"] = path;
    item.data["isVideo"] = isVideo;
    playlistManager->addItem(item);
}

void PlaylistPanel::addYouTube(const QString &url, const QString &title)
{
    const QVector<PlaylistItem> &items = playlistManager->getItems();
    for (int i = 0; i < items.size(); ++i) {
        const PlaylistItem &existing = items[i];
        if (existing.type == PlaylistItemType::YouTube) {
            const QString existingUrl = existing.data.value("url").toString(existing.reference);
            if (existingUrl == url) {
                highlightExistingItem(i);
                return;
            }
        }
    }

    PlaylistItem item;
    item.type = PlaylistItemType::YouTube;
    item.title = title.isEmpty() ? url : title;
    item.reference = url;
    item.data["url"] = url;
    playlistManager->addItem(item);
}

void PlaylistPanel::onRemoveClicked()
{
    int currentRow = playlistList->currentRow();
    if (currentRow >= 0) {
        playlistManager->removeItem(currentRow);
    }
}

void PlaylistPanel::onMoveUpClicked()
{
    int currentRow = playlistList->currentRow();
    if (currentRow > 0) {
        playlistManager->moveItem(currentRow, currentRow - 1);
        playlistList->setCurrentRow(currentRow - 1);
    }
}

void PlaylistPanel::onMoveDownClicked()
{
    int currentRow = playlistList->currentRow();
    if (currentRow >= 0 && currentRow < playlistList->count() - 1) {
        playlistManager->moveItem(currentRow, currentRow + 1);
        playlistList->setCurrentRow(currentRow + 1);
    }
}

void PlaylistPanel::onPlaylistChanged()
{
    refreshList();
}

int PlaylistPanel::rowForItemIndex(int itemIndex) const
{
    if (!playlistManager) {
        return -1;
    }
    const QVector<PlaylistItem> &items = playlistManager->getItems();
    if (itemIndex < 0 || itemIndex >= items.size()) {
        return -1;
    }

    int row = -1;
    for (int i = 0; i < items.size(); ++i) {
        if (!matchesFilter(items[i])) {
            continue;
        }
        ++row;
        if (i == itemIndex) {
            return row;
        }
    }
    return -1;
}

void PlaylistPanel::flashRow(int row)
{
    if (!playlistList || row < 0 || row >= playlistList->count()) {
        return;
    }

    const QColor baseColor = playlistList->palette().color(QPalette::Base);
    // Use a strong bright orange highlight so it is very visible.
    const QColor highlightColor = QColor(255, 165, 0);

    // Pulse for about 1.5 seconds with a slightly faster blink.
    const int flashCount = 5;      // number of on/off cycles
    const int intervalMs = 150;    // milliseconds between state changes

    for (int i = 0; i < flashCount * 2; ++i) {
        QTimer::singleShot(i * intervalMs, this,
                           [this, row, baseColor, highlightColor, i]() {
            if (!playlistList || row < 0 || row >= playlistList->count()) {
                return;
            }
            QListWidgetItem *item = playlistList->item(row);
            if (!item) {
                return;
            }
            const bool on = (i % 2 == 0);
            item->setBackground(on ? highlightColor : baseColor);
        });
    }

    QTimer::singleShot(flashCount * 2 * intervalMs + 10, this,
                       [this, row, baseColor]() {
        if (!playlistList || row < 0 || row >= playlistList->count()) {
            return;
        }
        QListWidgetItem *item = playlistList->item(row);
        if (item) {
            item->setBackground(baseColor);
        }
    });
}

void PlaylistPanel::highlightExistingItem(int itemIndex)
{
    if (!playlistManager) {
        return;
    }

    const QVector<PlaylistItem> &items = playlistManager->getItems();
    if (itemIndex < 0 || itemIndex >= items.size()) {
        return;
    }

    int row = rowForItemIndex(itemIndex);

    if (row < 0 && filterCombo) {
        const PlaylistItem &item = items[itemIndex];
        // Adjust filter so the existing item becomes visible, then recompute row.
        switch (item.type) {
        case PlaylistItemType::BibleVerse:
            filterCombo->setCurrentIndex(1); // Bible
            break;
        case PlaylistItemType::Song:
            filterCombo->setCurrentIndex(2); // Songs
            break;
        case PlaylistItemType::Media:
            filterCombo->setCurrentIndex(3); // Media
            break;
        case PlaylistItemType::YouTube:
            filterCombo->setCurrentIndex(6); // YouTube
            break;
        }

        row = rowForItemIndex(itemIndex);
    }

    if (row < 0) {
        return;
    }
    if (playlistList) {
        if (QListWidgetItem *item = playlistList->item(row)) {
            playlistList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
        }
    }
    flashRow(row);
}

bool PlaylistPanel::matchesFilter(const PlaylistItem &item) const
{
    switch (currentFilter) {
    case FilterMode::All:
        return true;
    case FilterMode::Bible:
        return item.type == PlaylistItemType::BibleVerse;
    case FilterMode::Songs:
        return item.type == PlaylistItemType::Song;
    case FilterMode::MediaAll:
        return item.type == PlaylistItemType::Media;
    case FilterMode::MediaVideos:
        return item.type == PlaylistItemType::Media && item.data.value("isVideo").toBool();
    case FilterMode::MediaImages:
        return item.type == PlaylistItemType::Media && !item.data.value("isVideo").toBool();
    case FilterMode::YouTube:
        return item.type == PlaylistItemType::YouTube;
    }
    return true;
}

void PlaylistPanel::refreshList()
{
    playlistList->clear();
    
    const QVector<PlaylistItem> &items = playlistManager->getItems();
    QStyle *style = this->style();
    for (const PlaylistItem &item : items) {
        if (!matchesFilter(item)) {
            continue;
        }
        QString displayText;
        QIcon icon;
        
        if (item.type == PlaylistItemType::BibleVerse) {
            displayText = QString("📖 %1").arg(item.displayText());
            icon = style->standardIcon(QStyle::SP_FileDialogContentsView);
        } else if (item.type == PlaylistItemType::Song) {
            displayText = QString("🎵 %1").arg(item.displayText());
            icon = style->standardIcon(QStyle::SP_MediaPlay);
        } else if (item.type == PlaylistItemType::Media) {
            bool isVideo = item.data.value("isVideo").toBool();
            displayText = QString("🖼️ %1").arg(item.displayText());
            icon = isVideo ? style->standardIcon(QStyle::SP_FileDialogDetailedView)
                           : style->standardIcon(QStyle::SP_FileIcon);
        } else if (item.type == PlaylistItemType::YouTube) {
            displayText = QString("▶️ %1").arg(item.displayText());
            icon = QIcon(":/icons/youtube_logo.png");
        } else {
            displayText = item.displayText();
            icon = style->standardIcon(QStyle::SP_FileIcon);
        }
        
        QListWidgetItem *listItem = new QListWidgetItem(icon, displayText);
        playlistList->addItem(listItem);
    }
}
