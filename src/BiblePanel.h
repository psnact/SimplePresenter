#ifndef BIBLEPANEL_H
#define BIBLEPANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include "BibleManager.h"

class BiblePanel : public QWidget
{
    Q_OBJECT

public:
    explicit BiblePanel(QWidget *parent = nullptr);
    ~BiblePanel();
    
    // Public method to activate a verse from playlist
    void activateVerse(const QString &reference);
    void refreshAvailableBibles();

    QString currentTranslationName() const { return bibleManager ? bibleManager->getCurrentTranslationAcronym() : QString(); }

signals:
    void verseSelected(const QString &reference, const QString &text);
    void addVerseToPlaylist(const QString &reference);

private slots:
    void onReferenceSearchChanged(const QString &text);
    void onReferenceTextEdited(const QString &text);
    void onTextSearchChanged(const QString &text);
    void onSearchResultClicked(QListWidgetItem *item);
    void onProjectClicked();
    void onNextVerse();
    void onPreviousVerse();
    void onBibleLoaded(const QString &translation);
    void onTranslationChanged(int index);
    void onContextMenuRequested(const QPoint &pos);

private:
    void setupUI();
    void loadBibles();
    void parseAndDisplayVerse(const QString &reference);
    void displayVerses(const QVector<BibleVerse> &verses);
    
    BibleManager *bibleManager;
    
    QComboBox *translationCombo;
    QLineEdit *referenceSearchEdit;
    QLineEdit *textSearchEdit;
    QListWidget *resultsList;
    QPushButton *projectButton;
    QPushButton *previousButton;
    QPushButton *nextButton;
    QCompleter *bookCompleter;
    
    QString currentBook;
    int currentChapter;
    int currentVerse;
    QVector<BibleVerse> currentVerses;
    QString defaultBiblePath;
};

#endif // BIBLEPANEL_H
