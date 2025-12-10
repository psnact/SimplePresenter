#include "BibleManager.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QDir>
#include <QRegularExpression>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

static const char *kDefaultBibleResources[] = {
    ":/bibles/AMP.xml",
    ":/bibles/ESV.xml",
    ":/bibles/MBB.xml",
    ":/bibles/MSG.xml",
    ":/bibles/NASB.xml",
    ":/bibles/NKJV.xml",
    ":/bibles/NLT.xml"
};

static void copyDefaultBiblesToDir(const QString &targetDir)
{
    if (targetDir.isEmpty()) {
        return;
    }

    QDir dir(targetDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return;
        }
    }

    const int count = sizeof(kDefaultBibleResources) / sizeof(kDefaultBibleResources[0]);
    for (int i = 0; i < count; ++i) {
        const QString resPath = QString::fromLatin1(kDefaultBibleResources[i]);
        QFileInfo resInfo(resPath);
        const QString fileName = resInfo.fileName();
        if (fileName.isEmpty()) {
            continue;
        }

        const QString dstPath = dir.filePath(fileName);
        if (QFile::exists(dstPath)) {
            continue;
        }

        QFile src(resPath);
        if (!src.open(QIODevice::ReadOnly)) {
            continue;
        }

        QFile dst(dstPath);
        if (!dst.open(QIODevice::WriteOnly)) {
            src.close();
            continue;
        }

        const QByteArray data = src.readAll();
        if (!data.isEmpty()) {
            dst.write(data);
        }
        dst.close();
        src.close();
    }
}

}

BibleManager::BibleManager(QObject *parent)
    : QObject(parent)
{
    // Initialize common book name aliases
    bookAliases["gen"] = "Genesis";
    bookAliases["exo"] = "Exodus";
    bookAliases["lev"] = "Leviticus";
    bookAliases["num"] = "Numbers";
    bookAliases["deu"] = "Deuteronomy";
    bookAliases["jos"] = "Joshua";
    bookAliases["jdg"] = "Judges";
    bookAliases["rut"] = "Ruth";
    bookAliases["1sa"] = "1 Samuel";
    bookAliases["2sa"] = "2 Samuel";
    bookAliases["1ki"] = "1 Kings";
    bookAliases["2ki"] = "2 Kings";
    bookAliases["1ch"] = "1 Chronicles";
    bookAliases["2ch"] = "2 Chronicles";
    bookAliases["ezr"] = "Ezra";
    bookAliases["neh"] = "Nehemiah";
    bookAliases["est"] = "Esther";
    bookAliases["job"] = "Job";
    bookAliases["psa"] = "Psalms";
    bookAliases["pro"] = "Proverbs";
    bookAliases["ecc"] = "Ecclesiastes";
    bookAliases["sng"] = "Song of Solomon";
    bookAliases["isa"] = "Isaiah";
    bookAliases["jer"] = "Jeremiah";
    bookAliases["lam"] = "Lamentations";
    bookAliases["ezk"] = "Ezekiel";
    bookAliases["dan"] = "Daniel";
    bookAliases["hos"] = "Hosea";
    bookAliases["jol"] = "Joel";
    bookAliases["amo"] = "Amos";
    bookAliases["oba"] = "Obadiah";
    bookAliases["jon"] = "Jonah";
    bookAliases["mic"] = "Micah";
    bookAliases["nam"] = "Nahum";
    bookAliases["hab"] = "Habakkuk";
    bookAliases["zep"] = "Zephaniah";
    bookAliases["hag"] = "Haggai";
    bookAliases["zec"] = "Zechariah";
    bookAliases["mal"] = "Malachi";
    bookAliases["mat"] = "Matthew";
    bookAliases["mrk"] = "Mark";
    bookAliases["luk"] = "Luke";
    bookAliases["jhn"] = "John";
    bookAliases["act"] = "Acts";
    bookAliases["rom"] = "Romans";
    bookAliases["1co"] = "1 Corinthians";
    bookAliases["2co"] = "2 Corinthians";
    bookAliases["gal"] = "Galatians";
    bookAliases["eph"] = "Ephesians";
    bookAliases["php"] = "Philippians";
    bookAliases["col"] = "Colossians";
    bookAliases["1th"] = "1 Thessalonians";
    bookAliases["2th"] = "2 Thessalonians";
    bookAliases["1ti"] = "1 Timothy";
    bookAliases["2ti"] = "2 Timothy";
    bookAliases["tit"] = "Titus";
    bookAliases["phm"] = "Philemon";
    bookAliases["heb"] = "Hebrews";
    bookAliases["jas"] = "James";
    bookAliases["1pe"] = "1 Peter";
    bookAliases["2pe"] = "2 Peter";
    bookAliases["1jn"] = "1 John";
    bookAliases["2jn"] = "2 John";
    bookAliases["3jn"] = "3 John";
    bookAliases["jud"] = "Jude";
    bookAliases["rev"] = "Revelation";
}

BibleManager::~BibleManager()
{
}

bool BibleManager::loadBible(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit bibleLoadError(QString("Cannot open file: %1").arg(filePath));
        return false;
    }
    
    books.clear();
    
    QXmlStreamReader xml(&file);
    QString currentBookName;
    int currentChapter = 0;
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement()) {
            // Support both formats: <XMLBIBLE> and <bible>
            if (xml.name() == QString("XMLBIBLE") || xml.name() == QString("bible")) {
                // Try biblename first (new format), then translation (old format)
                const QString biblename = xml.attributes().value("biblename").toString();
                const QString translationCode = xml.attributes().value("translation").toString();

                currentTranslation = biblename;
                if (currentTranslation.isEmpty()) {
                    currentTranslation = translationCode;
                }
                if (currentTranslation.isEmpty()) {
                    currentTranslation = QFileInfo(filePath).baseName();
                }

                // Derive a short acronym for the translation (e.g., NKJV, NASB, AMP)
                currentTranslationAcronym.clear();

                auto normalizeLetters = [](const QString &input) {
                    QString lettersOnly;
                    for (const QChar &ch : input) {
                        if (ch.isLetter()) {
                            lettersOnly.append(ch.toUpper());
                        }
                    }
                    return lettersOnly;
                };

                // 1) Prefer the translation attribute if present (often already an acronym)
                QString candidate = normalizeLetters(translationCode.trimmed());
                if (!candidate.isEmpty() && candidate.length() <= 12) {
                    currentTranslationAcronym = candidate;
                }

                auto deriveFromName = [&](const QString &source) -> QString {
                    if (source.isEmpty()) {
                        return QString();
                    }

                    // Use text inside the last parentheses if available, e.g. "New King James (NKJV)"
                    int openIdx = source.lastIndexOf('(');
                    int closeIdx = source.lastIndexOf(')');
                    if (openIdx != -1 && closeIdx > openIdx + 1) {
                        QString inside = normalizeLetters(source.mid(openIdx + 1, closeIdx - openIdx - 1));
                        if (!inside.isEmpty() && inside.length() <= 12) {
                            return inside;
                        }
                    }

                    // Otherwise, take the last word and use its letters
                    QRegularExpression wordRe("[A-Za-z]+");
                    QRegularExpressionMatchIterator it = wordRe.globalMatch(source);
                    QString lastToken;
                    while (it.hasNext()) {
                        lastToken = it.next().captured(0);
                    }
                    if (!lastToken.isEmpty()) {
                        QString letters = normalizeLetters(lastToken);
                        if (letters.length() >= 2 && letters.length() <= 12) {
                            return letters;
                        }
                    }

                    return QString();
                };

                // 2) Fallback: derive from the file base name (often an acronym like AMP, NKJV, NASB)
                if (currentTranslationAcronym.isEmpty()) {
                    currentTranslationAcronym = deriveFromName(QFileInfo(filePath).baseName());
                }

                // 3) Fallback: derive from the human-readable translation name
                if (currentTranslationAcronym.isEmpty()) {
                    currentTranslationAcronym = deriveFromName(currentTranslation);
                }

                // 4) Final fallback: use initials of each word
                if (currentTranslationAcronym.isEmpty()) {
                    QStringList words = currentTranslation.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    QString initials;
                    for (const QString &w : words) {
                        if (!w.isEmpty()) {
                            initials.append(w.at(0).toUpper());
                            if (initials.length() >= 8) {
                                break;
                            }
                        }
                    }
                    currentTranslationAcronym = initials;
                }

                if (currentTranslationAcronym.isEmpty()) {
                    currentTranslationAcronym = currentTranslation;
                }
            }
            // Support both formats: <BIBLEBOOK> and <book>
            else if (xml.name() == QString("BIBLEBOOK") || xml.name() == QString("book")) {
                // Try bname first (new format), then name (old format)
                currentBookName = xml.attributes().value("bname").toString();
                if (currentBookName.isEmpty()) {
                    currentBookName = xml.attributes().value("name").toString();
                }
                if (!currentBookName.isEmpty() && !books.contains(currentBookName)) {
                    BibleBook book;
                    book.name = currentBookName;
                    books[currentBookName] = book;
                }
            }
            // Support both formats: <CHAPTER> and <chapter>
            else if (xml.name() == QString("CHAPTER") || xml.name() == QString("chapter")) {
                // Try cnumber first (new format), then number (old format)
                currentChapter = xml.attributes().value("cnumber").toInt();
                if (currentChapter == 0) {
                    currentChapter = xml.attributes().value("number").toInt();
                }
            }
            // Support both formats: <VERS> and <verse>
            else if (xml.name() == QString("VERS") || xml.name() == QString("verse")) {
                // Try vnumber first (new format), then number (old format)
                int verseNumber = xml.attributes().value("vnumber").toInt();
                if (verseNumber == 0) {
                    verseNumber = xml.attributes().value("number").toInt();
                }
                QString verseText = xml.readElementText().trimmed();
                
                if (!currentBookName.isEmpty() && currentChapter > 0 && verseNumber > 0) {
                    books[currentBookName].chapters[currentChapter][verseNumber] = verseText;
                }
            }
        }
    }
    
    if (xml.hasError()) {
        emit bibleLoadError(QString("XML parse error: %1").arg(xml.errorString()));
        return false;
    }
    
    file.close();
    
    emit bibleLoaded(currentTranslation);
    return true;
}

QStringList BibleManager::getAvailableBibles() const
{
    QStringList bibles;
    const QString dirPath = bibleDirectory();
    QDir dir(dirPath);
    QStringList filters;
    filters << "*.xml";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    if (files.isEmpty()) {
        copyDefaultBiblesToDir(dirPath);
        QDir refreshedDir(dirPath);
        files = refreshedDir.entryInfoList(filters, QDir::Files);
    }
    for (const QFileInfo &fileInfo : files) {
        bibles.append(fileInfo.absoluteFilePath());
    }
    
    return bibles;
}

QString BibleManager::bibleDirectory()
{
#ifdef Q_OS_WIN
    const QString legacyPath = QStringLiteral("C:/SimplePresenter/Bible");
#endif

    // Cross-platform application data location, e.g.:
    // Windows: %LOCALAPPDATA%/SimplePresenter/SimplePresenter/Bible
    // macOS:   ~/Library/Application Support/SimplePresenter/Bible
    // Linux:   ~/.local/share/SimplePresenter/Bible
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString appDataPath;
    if (!baseDir.isEmpty()) {
        QDir dir(baseDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        dir.mkpath("Bible");
        dir.cd("Bible");
        appDataPath = dir.absolutePath();
    }

#ifdef Q_OS_WIN
    // On Windows, prefer the AppData Bible folder when it contains any XMLs.
    // Fall back to the legacy C:/SimplePresenter/Bible folder when that is
    // where existing Bibles live.
    QStringList filters;
    filters << "*.xml";

    bool appDataHasBibles = !appDataPath.isEmpty() &&
        !QDir(appDataPath).entryInfoList(filters, QDir::Files).isEmpty();
    bool legacyHasBibles = QDir(legacyPath).exists() &&
        !QDir(legacyPath).entryInfoList(filters, QDir::Files).isEmpty();

    if (appDataHasBibles) {
        return appDataPath;
    }
    if (legacyHasBibles) {
        return legacyPath;
    }

    // No Bibles yet: default to AppData path when available, otherwise legacy
    if (!appDataPath.isEmpty()) {
        return appDataPath;
    }
    return legacyPath;
#else
    return appDataPath;
#endif
}

QStringList BibleManager::getBookNames() const
{
    return books.keys();
}

QString BibleManager::getVerse(const QString &book, int chapter, int verse) const
{
    QString normalizedBook = normalizeBookName(book);
    
    if (books.contains(normalizedBook)) {
        const BibleBook &bibleBook = books[normalizedBook];
        if (bibleBook.chapters.contains(chapter)) {
            if (bibleBook.chapters[chapter].contains(verse)) {
                return bibleBook.chapters[chapter][verse];
            }
        }
    }
    
    return QString();
}

QVector<BibleVerse> BibleManager::getVerses(const QString &book, int chapter, int startVerse, int endVerse) const
{
    QVector<BibleVerse> verses;
    QString normalizedBook = normalizeBookName(book);
    
    if (books.contains(normalizedBook)) {
        const BibleBook &bibleBook = books[normalizedBook];
        if (bibleBook.chapters.contains(chapter)) {
            for (int v = startVerse; v <= endVerse; ++v) {
                if (bibleBook.chapters[chapter].contains(v)) {
                    BibleVerse verse;
                    verse.book = normalizedBook;
                    verse.chapter = chapter;
                    verse.verse = v;
                    verse.text = bibleBook.chapters[chapter][v];
                    verses.append(verse);
                }
            }
        }
    }
    
    return verses;
}

bool BibleManager::parseReference(const QString &reference, QString &book, int &chapter, int &startVerse, int &endVerse) const
{
    // Parse formats like:
    // "John 3:16"
    // "John 3:16-18"
    // "1 John 2:1"
    
    QRegularExpression re(R"(^((?:\d\s)?[A-Za-z]+)\s+(\d+):(\d+)(?:-(\d+))?$)");
    QRegularExpressionMatch match = re.match(reference.trimmed());
    
    if (match.hasMatch()) {
        book = normalizeBookName(match.captured(1).trimmed());
        chapter = match.captured(2).toInt();
        startVerse = match.captured(3).toInt();
        
        if (match.captured(4).isEmpty()) {
            endVerse = startVerse;
        } else {
            endVerse = match.captured(4).toInt();
        }
        
        return true;
    }
    
    return false;
}

QVector<BibleVerse> BibleManager::search(const QString &searchText, int maxResults) const
{
    QVector<BibleVerse> results;
    QString lowerSearch = searchText.toLower();
    
    for (auto bookIt = books.constBegin(); bookIt != books.constEnd(); ++bookIt) {
        const BibleBook &book = bookIt.value();
        
        for (auto chapterIt = book.chapters.constBegin(); chapterIt != book.chapters.constEnd(); ++chapterIt) {
            int chapterNum = chapterIt.key();
            
            for (auto verseIt = chapterIt.value().constBegin(); verseIt != chapterIt.value().constEnd(); ++verseIt) {
                int verseNum = verseIt.key();
                QString verseText = verseIt.value();
                
                if (verseText.toLower().contains(lowerSearch)) {
                    BibleVerse verse;
                    verse.book = book.name;
                    verse.chapter = chapterNum;
                    verse.verse = verseNum;
                    verse.text = verseText;
                    results.append(verse);
                    
                    if (results.size() >= maxResults) {
                        return results;
                    }
                }
            }
        }
    }
    
    return results;
}

int BibleManager::getChapterCount(const QString &book) const
{
    QString normalizedBook = normalizeBookName(book);
    
    if (books.contains(normalizedBook)) {
        return books[normalizedBook].chapters.size();
    }
    
    return 0;
}

int BibleManager::getVerseCount(const QString &book, int chapter) const
{
    QString normalizedBook = normalizeBookName(book);
    
    if (books.contains(normalizedBook)) {
        const BibleBook &bibleBook = books[normalizedBook];
        if (bibleBook.chapters.contains(chapter)) {
            return bibleBook.chapters[chapter].size();
        }
    }
    
    return 0;
}

QStringList BibleManager::autocompleteBook(const QString &partial) const
{
    QStringList matches;
    QString lowerPartial = partial.toLower();
    
    // Check full names
    for (const QString &bookName : books.keys()) {
        if (bookName.toLower().startsWith(lowerPartial)) {
            matches.append(bookName);
        }
    }
    
    // Check aliases
    for (auto it = bookAliases.constBegin(); it != bookAliases.constEnd(); ++it) {
        if (it.key().startsWith(lowerPartial) && books.contains(it.value())) {
            if (!matches.contains(it.value())) {
                matches.append(it.value());
            }
        }
    }
    
    return matches;
}

QString BibleManager::normalizeBookName(const QString &name) const
{
    QString lower = name.toLower().trimmed();
    
    // Check if it's an alias
    if (bookAliases.contains(lower)) {
        return bookAliases[lower];
    }
    
    // Check if it matches a book name (case-insensitive)
    for (const QString &bookName : books.keys()) {
        if (bookName.toLower() == lower) {
            return bookName;
        }
    }
    
    // Return as-is if no match
    return name;
}
