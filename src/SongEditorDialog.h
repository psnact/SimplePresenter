#ifndef SONGEDITORDIALOG_H
#define SONGEDITORDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include "SongManager.h"

class SongEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SongEditorDialog(QWidget *parent = nullptr);
    explicit SongEditorDialog(const Song &song, QWidget *parent = nullptr);
    
    Song getSong() const;
    
private slots:
    void onAccepted();
    void onRejected();

private:
    void setupUI();
    void loadSong(const Song &song);
    
    QLineEdit *titleEdit;
    QLineEdit *authorEdit;
    QLineEdit *copyrightEdit;
    QLineEdit *ccliEdit;
    QTextEdit *sectionTextEdit;
    
    Song currentSong;
};

#endif // SONGEDITORDIALOG_H
