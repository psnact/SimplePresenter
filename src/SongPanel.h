#ifndef SONGPANEL_H
#define SONGPANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include "SongManager.h"

class SongPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SongPanel(QWidget *parent = nullptr);
    ~SongPanel();
    
    // Public method to activate a song from playlist
    void activateSong(const QString &songTitle);

signals:
    void sectionSelected(const QString &songTitle, const QString &sectionText);
    void addSongToPlaylist(const QString &songTitle);
    void addSectionToPlaylist(const QString &songTitle, const QString &sectionText);

private slots:
    void onSearchTextChanged(const QString &text);
    void onSongClicked(QListWidgetItem *item);
    void onSectionClicked(QListWidgetItem *item);
    void onProjectClicked();
    void onNextSection();
    void onPreviousSection();
    void onRefreshClicked();
    void onAddSongClicked();
    void onEditSongClicked();
    void onDeleteSongClicked();
    void onContextMenuRequested(const QPoint &pos);
    void onSongContextMenuRequested(const QPoint &pos);

private:
    void setupUI();
    void loadSongs();
    void displaySong(const Song &song);
    
    SongManager *songManager;
    
    QLineEdit *searchEdit;
    QListWidget *songsList;
    QListWidget *sectionsList;
    QPushButton *projectButton;
    QPushButton *previousButton;
    QPushButton *nextButton;
    QPushButton *refreshButton;
    QPushButton *addSongButton;
    QPushButton *editSongButton;
    QPushButton *deleteSongButton;
    
    QString currentSongTitle;
    Song currentSong;
    int currentSectionIndex;
};

#endif // SONGPANEL_H
