#ifndef BIBLEMANAGER_H
#define BIBLEMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QStringList>

struct BibleVerse {
    QString book;
    int chapter;
    int verse;
    QString text;
    
    QString reference() const {
        return QString("%1 %2:%3").arg(book).arg(chapter).arg(verse);
    }
};

struct BibleBook {
    QString name;
    QMap<int, QMap<int, QString>> chapters; // chapter -> verse -> text
};

class BibleManager : public QObject
{
    Q_OBJECT

public:
    explicit BibleManager(QObject *parent = nullptr);
    ~BibleManager();
    
    // Load Bible from XML file
    bool loadBible(const QString &filePath);
    
    // Get list of available Bibles
    QStringList getAvailableBibles() const;
    
    // Get current Bible translation name
    QString getCurrentTranslation() const { return currentTranslation; }
    // Get short acronym for the current Bible translation (e.g., NKJV, NASB, AMP)
    QString getCurrentTranslationAcronym() const { return currentTranslationAcronym; }
    
    // Get list of book names
    QStringList getBookNames() const;
    
    // Get verse text
    QString getVerse(const QString &book, int chapter, int verse) const;
    
    // Get multiple verses
    QVector<BibleVerse> getVerses(const QString &book, int chapter, int startVerse, int endVerse) const;
    
    // Parse reference string (e.g., "John 3:16" or "John 3:16-18")
    bool parseReference(const QString &reference, QString &book, int &chapter, int &startVerse, int &endVerse) const;
    
    // Search for verses containing text
    QVector<BibleVerse> search(const QString &searchText, int maxResults = 50) const;
    
    // Get chapter count for a book
    int getChapterCount(const QString &book) const;
    
    // Get verse count for a chapter
    int getVerseCount(const QString &book, int chapter) const;
    
    // Autocomplete book names
    QStringList autocompleteBook(const QString &partial) const;

    static QString bibleDirectory();

signals:
    void bibleLoaded(const QString &translation);
    void bibleLoadError(const QString &error);

private:
    QString normalizeBookName(const QString &name) const;
    QString currentTranslation;
    QString currentTranslationAcronym;
    QMap<QString, BibleBook> books;
    QMap<QString, QString> bookAliases; // Abbreviations to full names
};

#endif // BIBLEMANAGER_H
