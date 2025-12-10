#include "SongPanel.h"
#include "SongEditorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QMenu>
#include <QMessageBox>
#include <QFile>
#include <QXmlStreamWriter>
#include <QDir>
#include <QStandardPaths>

SongPanel::SongPanel(QWidget *parent)
    : QWidget(parent)
    , songManager(new SongManager(this))
    , currentSectionIndex(-1)
{
    setupUI();
    loadSongs();
}

SongPanel::~SongPanel()
{
}

static QString songsDirectory()
{
#ifdef Q_OS_MACOS
    // On macOS, store songs in the per-user AppData location
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    dir.mkpath("songs");
    dir.cd("songs");
    return dir.absolutePath();
#else
    // On other platforms, keep existing relative data/songs behavior
    QDir dir("data/songs");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absolutePath();
#endif
}

void SongPanel::activateSong(const QString &songTitle)
{
    // Find the song in the list
    for (int i = 0; i < songsList->count(); ++i) {
        QListWidgetItem *item = songsList->item(i);
        if (item->text() == songTitle) {
            songsList->setCurrentItem(item);
            onSongClicked(item);  // This will load the sections
            break;
        }
    }
}

void SongPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // Search box
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search songs...");
    searchLayout->addWidget(searchEdit);
    
    // Refresh button
    refreshButton = new QPushButton("Refresh");
    searchLayout->addWidget(refreshButton);
    
    // Song management buttons
    addSongButton = new QPushButton("Add Song");
    searchLayout->addWidget(addSongButton);
    
    editSongButton = new QPushButton("Edit Song");
    editSongButton->setEnabled(false);
    searchLayout->addWidget(editSongButton);
    
    deleteSongButton = new QPushButton("Delete Song");
    deleteSongButton->setEnabled(false);
    searchLayout->addWidget(deleteSongButton);
    
    mainLayout->addLayout(searchLayout);
    
    // Splitter for songs list and sections
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    // Songs list
    QWidget *songsWidget = new QWidget();
    QVBoxLayout *songsLayout = new QVBoxLayout(songsWidget);
    songsLayout->setContentsMargins(0, 0, 0, 0);
    songsLayout->addWidget(new QLabel("Songs:"));
    songsList = new QListWidget();
    songsList->setDragEnabled(true);
    songsList->setContextMenuPolicy(Qt::CustomContextMenu);
    songsLayout->addWidget(songsList);
    splitter->addWidget(songsWidget);
    
    // Connect context menu for songs
    connect(songsList, &QListWidget::customContextMenuRequested,
            this, &SongPanel::onSongContextMenuRequested);
    
    // Sections list
    QWidget *sectionsWidget = new QWidget();
    QVBoxLayout *sectionsLayout = new QVBoxLayout(sectionsWidget);
    sectionsLayout->setContentsMargins(0, 0, 0, 0);
    sectionsLayout->addWidget(new QLabel("Sections:"));
    sectionsList = new QListWidget();
    sectionsList->setDragEnabled(true);
    sectionsList->setContextMenuPolicy(Qt::CustomContextMenu);
    sectionsLayout->addWidget(sectionsList);
    splitter->addWidget(sectionsWidget);
    
    // Connect context menu
    connect(sectionsList, &QListWidget::customContextMenuRequested,
            this, &SongPanel::onContextMenuRequested);
    
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
    
    // Control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    previousButton = new QPushButton("Previous");
    previousButton->setEnabled(false);
    buttonLayout->addWidget(previousButton);
    
    projectButton = new QPushButton("Project Section");
    projectButton->setEnabled(false);
    buttonLayout->addWidget(projectButton);
    
    nextButton = new QPushButton("Next");
    nextButton->setEnabled(false);
    buttonLayout->addWidget(nextButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(searchEdit, &QLineEdit::textChanged,
            this, &SongPanel::onSearchTextChanged);
    connect(songsList, &QListWidget::itemClicked,
            this, &SongPanel::onSongClicked);
    connect(sectionsList, &QListWidget::itemClicked,
            this, &SongPanel::onSectionClicked);
    connect(sectionsList, &QListWidget::itemDoubleClicked,
            this, &SongPanel::onProjectClicked);
    connect(projectButton, &QPushButton::clicked,
            this, &SongPanel::onProjectClicked);
    connect(previousButton, &QPushButton::clicked,
            this, &SongPanel::onPreviousSection);
    connect(nextButton, &QPushButton::clicked,
            this, &SongPanel::onNextSection);
    connect(refreshButton, &QPushButton::clicked,
            this, &SongPanel::onRefreshClicked);
    connect(addSongButton, &QPushButton::clicked,
            this, &SongPanel::onAddSongClicked);
    connect(editSongButton, &QPushButton::clicked,
            this, &SongPanel::onEditSongClicked);
    connect(deleteSongButton, &QPushButton::clicked,
            this, &SongPanel::onDeleteSongClicked);
}

void SongPanel::loadSongs()
{
    songManager->loadSongsFromDirectory(songsDirectory());
    
    songsList->clear();
    QStringList titles = songManager->getSongTitles();
    songsList->addItems(titles);
}

void SongPanel::onSearchTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        songsList->clear();
        songsList->addItems(songManager->getSongTitles());
    } else {
        QStringList results = songManager->searchSongs(text);
        songsList->clear();
        songsList->addItems(results);
    }
}

void SongPanel::onSongClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    QString songTitle = item->text();
    Song song = songManager->getSong(songTitle);
    
    if (!song.title.isEmpty()) {
        currentSongTitle = songTitle;
        currentSong = song;
        displaySong(song);
        
        // Enable edit/delete buttons when a song is selected
        editSongButton->setEnabled(true);
        deleteSongButton->setEnabled(true);
    }
}

void SongPanel::onSectionClicked(QListWidgetItem *item)
{
    
    currentSectionIndex = item->data(Qt::UserRole).toInt();
    projectButton->setEnabled(true);
    
    // Update next/previous button states
    int currentRow = sectionsList->currentRow();
    previousButton->setEnabled(currentRow > 0);
    nextButton->setEnabled(currentRow < sectionsList->count() - 1);
}

void SongPanel::onProjectClicked()
{
    if (currentSectionIndex < 0 || currentSectionIndex >= currentSong.sections.size()) {
        return;
    }
    
    const SongSection &section = currentSong.sections[currentSectionIndex];
    QString sectionText = section.text();
    
    emit sectionSelected(currentSongTitle, sectionText);
}

void SongPanel::onNextSection()
{
    int currentRow = sectionsList->currentRow();
    if (currentRow < sectionsList->count() - 1) {
        sectionsList->setCurrentRow(currentRow + 1);
        onSectionClicked(sectionsList->currentItem());
        onProjectClicked();  // Project the next section
    }
}

void SongPanel::onPreviousSection()
{
    int currentRow = sectionsList->currentRow();
    if (currentRow > 0) {
        sectionsList->setCurrentRow(currentRow - 1);
        onSectionClicked(sectionsList->currentItem());
        onProjectClicked();  // Project the previous section
    }
}

void SongPanel::onRefreshClicked()
{
    loadSongs();
}

void SongPanel::onAddSongClicked()
{
    SongEditorDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Song newSong = dialog.getSong();
        
        // Save song to XML file in the songs directory
        const QString baseDir = songsDirectory();
        QDir dir(baseDir);
        const QString fileName = dir.filePath(QString("%1.xml").arg(newSong.title));
        
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            QXmlStreamWriter xml(&file);
            xml.setAutoFormatting(true);
            xml.writeStartDocument();
            
            xml.writeStartElement("song");
            xml.writeTextElement("title", newSong.title);
            xml.writeTextElement("author", newSong.author);
            xml.writeTextElement("copyright", newSong.copyright);
            xml.writeTextElement("ccli", newSong.ccli);
            
            xml.writeStartElement("lyrics");
            for (const SongSection &section : newSong.sections) {
                xml.writeStartElement("section");
                xml.writeAttribute("type", section.type);
                for (const QString &line : section.lines) {
                    xml.writeTextElement("line", line);
                }
                xml.writeEndElement(); // section
            }
            xml.writeEndElement(); // lyrics
            
            xml.writeEndElement(); // song
            xml.writeEndDocument();
            
            file.close();
            
            QMessageBox::information(this, "Success", "Song created successfully!");
            loadSongs();  // Refresh the list
        } else {
            QMessageBox::warning(this, "Error", "Failed to save song file");
        }
    }
}

void SongPanel::onEditSongClicked()
{
    if (currentSongTitle.isEmpty()) return;
    
    SongEditorDialog dialog(currentSong, this);
    if (dialog.exec() == QDialog::Accepted) {
        Song editedSong = dialog.getSong();
        
        const QString baseDir = songsDirectory();

        // Delete old file if title changed
        if (editedSong.title != currentSongTitle) {
            const QString oldFileName = QDir(baseDir).filePath(QString("%1.xml").arg(currentSongTitle));
            QFile::remove(oldFileName);
        }
        
        // Save song to XML file
        const QString fileName = QDir(baseDir).filePath(QString("%1.xml").arg(editedSong.title));
        
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            QXmlStreamWriter xml(&file);
            xml.setAutoFormatting(true);
            xml.writeStartDocument();
            
            xml.writeStartElement("song");
            xml.writeTextElement("title", editedSong.title);
            xml.writeTextElement("author", editedSong.author);
            xml.writeTextElement("copyright", editedSong.copyright);
            xml.writeTextElement("ccli", editedSong.ccli);
            
            xml.writeStartElement("lyrics");
            for (const SongSection &section : editedSong.sections) {
                xml.writeStartElement("section");
                xml.writeAttribute("type", section.type);
                for (const QString &line : section.lines) {
                    xml.writeTextElement("line", line);
                }
                xml.writeEndElement(); // section
            }
            xml.writeEndElement(); // lyrics
            
            xml.writeEndElement(); // song
            xml.writeEndDocument();
            
            file.close();
            
            QMessageBox::information(this, "Success", "Song updated successfully!");
            loadSongs();  // Refresh the list
            sectionsList->clear();
            currentSongTitle.clear();
            editSongButton->setEnabled(false);
            deleteSongButton->setEnabled(false);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save song file");
        }
    }
}

void SongPanel::onDeleteSongClicked()
{
    if (currentSongTitle.isEmpty()) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Song",
        QString("Are you sure you want to delete '%1'?").arg(currentSongTitle),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Delete the song file from the songs directory
        const QString baseDir = songsDirectory();
        const QString filePath = QDir(baseDir).filePath(QString("%1.xml").arg(currentSongTitle));
        if (QFile::remove(filePath)) {
            QMessageBox::information(this, "Success", "Song deleted successfully");
            loadSongs();  // Refresh the list
            sectionsList->clear();
            currentSongTitle.clear();
            editSongButton->setEnabled(false);
            deleteSongButton->setEnabled(false);
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete song file");
        }
    }
}

void SongPanel::displaySong(const Song &song)
{
    sectionsList->clear();
    
    for (int i = 0; i < song.sections.size(); ++i) {
        const SongSection &section = song.sections[i];
        
        // Display full section text (all lines joined)
        QString displayText = section.text();
        
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, i);
        sectionsList->addItem(item);
    }
    
    if (!song.sections.isEmpty()) {
        sectionsList->setCurrentRow(0);
        currentSectionIndex = 0;
        projectButton->setEnabled(true);
    }
}

void SongPanel::onContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = sectionsList->itemAt(pos);
    if (!item) return;
    
    QMenu contextMenu(this);
    QAction *addToPlaylistAction = contextMenu.addAction("Add to Playlist");
    QAction *projectAction = contextMenu.addAction("Project");
    
    QAction *selectedAction = contextMenu.exec(sectionsList->mapToGlobal(pos));
    
    if (selectedAction == addToPlaylistAction) {
        int sectionIndex = item->data(Qt::UserRole).toInt();
        if (sectionIndex >= 0 && sectionIndex < currentSong.sections.size()) {
            QString sectionText = currentSong.sections[sectionIndex].text();
            emit addSectionToPlaylist(currentSongTitle, sectionText);
        }
    } else if (selectedAction == projectAction) {
        onSectionClicked(item);
        onProjectClicked();
    }
}

void SongPanel::onSongContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = songsList->itemAt(pos);
    if (!item) return;
    
    QMenu contextMenu(this);
    QAction *addToPlaylistAction = contextMenu.addAction("Add to Playlist");
    
    QAction *selectedAction = contextMenu.exec(songsList->mapToGlobal(pos));
    
    if (selectedAction == addToPlaylistAction) {
        QString songTitle = item->text();
        emit addSongToPlaylist(songTitle);
    }
}
