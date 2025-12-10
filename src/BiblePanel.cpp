#include "BiblePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCompleter>
#include <QStringListModel>
#include <QTimer>
#include <QFileInfo>
#include <QFile>
#include <QSettings>
#include <QMenu>
#include <QToolButton>
#include <QSignalBlocker>

BiblePanel::BiblePanel(QWidget *parent)
    : QWidget(parent)
    , bibleManager(new BibleManager(this))
    , currentChapter(0)
    , currentVerse(0)
    , bookCompleter(nullptr)
{
    setupUI();

    QSettings settings("SimplePresenter", "SimplePresenter");
    settings.beginGroup("ProjectionCanvas");
    defaultBiblePath = settings.value("defaultBibleTranslationPath").toString();
    settings.endGroup();
    
    // Connect signal BEFORE loading Bibles so we catch the first load
    connect(bibleManager, &BibleManager::bibleLoaded,
            this, &BiblePanel::onBibleLoaded);
    
    loadBibles();
}

BiblePanel::~BiblePanel()
{
}

void BiblePanel::activateVerse(const QString &reference)
{
    // Set the reference in the search box
    referenceSearchEdit->setText(reference);
    
    // This will trigger onReferenceSearchChanged which will:
    // 1. Load all following verses in the chapter
    // 2. Display them in the results list
    
    // Now project the verse immediately
    QString book;
    int chapter, startVerse, endVerse;
    if (bibleManager->parseReference(reference, book, chapter, startVerse, endVerse)) {
        QString verseText = bibleManager->getVerse(book, chapter, startVerse);
        if (!verseText.isEmpty()) {
            emit verseSelected(reference, verseText);
        }
    }
}

void BiblePanel::onReferenceTextEdited(const QString &text)
{
    QString fixed = text;

    int firstLetterIndex = -1;
    for (int i = 0; i < fixed.length(); ++i) {
        if (fixed[i].isLetter()) {
            firstLetterIndex = i;
            break;
        }
    }

    if (firstLetterIndex >= 0) {
        QChar c = fixed[firstLetterIndex];
        if (c.isLower()) {
            fixed[firstLetterIndex] = c.toUpper();
        }
    }

    if (fixed != text) {
        QSignalBlocker blocker(referenceSearchEdit);
        int cursorPos = referenceSearchEdit->cursorPosition();
        referenceSearchEdit->setText(fixed);
        referenceSearchEdit->setCursorPosition(cursorPos);
    }

    onReferenceSearchChanged(fixed);
}

void BiblePanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Translation selector
    QHBoxLayout *translationLayout = new QHBoxLayout();
    translationLayout->addWidget(new QLabel("Translation:"));
    translationCombo = new QComboBox();
    translationCombo->setMaxVisibleItems(10); // Limit dropdown height
    translationCombo->setMaximumWidth(200); // Limit width
    translationLayout->addWidget(translationCombo);
    translationLayout->addStretch(); // Push combo box to the left
    mainLayout->addLayout(translationLayout);
    
    // Reference search box
    QLabel *refSearchLabel = new QLabel("Search by Reference:");
    mainLayout->addWidget(refSearchLabel);
    
    referenceSearchEdit = new QLineEdit();
    referenceSearchEdit->setPlaceholderText("e.g., John 3:16 or John 3:16-18");
    mainLayout->addWidget(referenceSearchEdit);
    
    // Text search box
    QLabel *textSearchLabel = new QLabel("Search in Text:");
    mainLayout->addWidget(textSearchLabel);
    
    textSearchEdit = new QLineEdit();
    textSearchEdit->setPlaceholderText("Search for words in Bible text...");
    mainLayout->addWidget(textSearchEdit);
    
    // Results list
    resultsList = new QListWidget();
    resultsList->setDragEnabled(true);
    resultsList->setContextMenuPolicy(Qt::CustomContextMenu);
    mainLayout->addWidget(resultsList);
    
    // Connect context menu
    connect(resultsList, &QListWidget::customContextMenuRequested,
            this, &BiblePanel::onContextMenuRequested);
    
    // Control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    previousButton = new QPushButton("Previous");
    previousButton->setEnabled(false);
    buttonLayout->addWidget(previousButton);
    
    projectButton = new QPushButton("Project");
    projectButton->setEnabled(false);
    buttonLayout->addWidget(projectButton);
    
    nextButton = new QPushButton("Next");
    nextButton->setEnabled(false);
    buttonLayout->addWidget(nextButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(referenceSearchEdit, &QLineEdit::textChanged,
            this, &BiblePanel::onReferenceTextEdited);
    connect(textSearchEdit, &QLineEdit::textChanged,
            this, &BiblePanel::onTextSearchChanged);
    connect(resultsList, &QListWidget::itemClicked,
            this, &BiblePanel::onSearchResultClicked);
    connect(resultsList, &QListWidget::itemDoubleClicked,
            this, &BiblePanel::onProjectClicked);  // Double-click to project
    connect(projectButton, &QPushButton::clicked,
            this, &BiblePanel::onProjectClicked);
    connect(nextButton, &QPushButton::clicked,
            this, &BiblePanel::onNextVerse);
    connect(previousButton, &QPushButton::clicked,
            this, &BiblePanel::onPreviousVerse);
    connect(translationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BiblePanel::onTranslationChanged);
}

void BiblePanel::loadBibles()
{
    QStringList bibles = bibleManager->getAvailableBibles();
    QString fallbackPath;

    {
        QSignalBlocker blocker(translationCombo);
        translationCombo->clear();

        for (const QString &biblePath : bibles) {
            QFileInfo info(biblePath);
            QString label = info.baseName();
            if (label.isEmpty()) {
                label = biblePath;
            }
            translationCombo->addItem(label, biblePath);
            if (fallbackPath.isEmpty()) {
                fallbackPath = biblePath;
            }
        }

        QString targetPath = defaultBiblePath;
        if (targetPath.isEmpty() || !QFile::exists(targetPath)) {
            targetPath = fallbackPath;
        }

        if (translationCombo->count() > 0) {
            int index = translationCombo->findData(targetPath);
            if (index < 0) {
                index = 0;
            }
            translationCombo->setCurrentIndex(index);
        }
    }

    if (translationCombo->count() > 0) {
        QString selectedPath = translationCombo->currentData().toString();
        defaultBiblePath = selectedPath;
        if (!selectedPath.isEmpty()) {
            bibleManager->loadBible(selectedPath);
        }
    }
}

void BiblePanel::onReferenceSearchChanged(const QString &text)
{
    if (text.isEmpty()) {
        resultsList->clear();
        return;
    }
    
    // Parse as reference only
    QString book;
    int chapter, startVerse, endVerse;
    
    if (bibleManager->parseReference(text, book, chapter, startVerse, endVerse)) {
        // Get from startVerse to end of chapter (999)
        QVector<BibleVerse> verses = bibleManager->getVerses(book, chapter, startVerse, 999);
        currentVerses = verses;
        if (!verses.isEmpty()) {
            currentBook = book;
            currentChapter = chapter;
            currentVerse = startVerse;
        }
        displayVerses(verses);
    } else {
        resultsList->clear();
    }
}

void BiblePanel::onTextSearchChanged(const QString &text)
{
    if (text.isEmpty()) {
        resultsList->clear();
        currentVerses.clear();
        projectButton->setEnabled(false);
        nextButton->setEnabled(false);
        previousButton->setEnabled(false);
        return;
    }
    
    // Search in Bible text
    QVector<BibleVerse> results = bibleManager->search(text, 50);

    // Store results so onProjectClicked can find the verse text
    currentVerses = results;

    // Update navigation/buttons state
    if (!results.isEmpty()) {
        projectButton->setEnabled(true);
        nextButton->setEnabled(results.size() > 1);
        previousButton->setEnabled(false);
    } else {
        projectButton->setEnabled(false);
        nextButton->setEnabled(false);
        previousButton->setEnabled(false);
    }

    displayVerses(results);
}

void BiblePanel::onSearchResultClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    // Just select the verse, don't reload the list
    QString reference = item->data(Qt::UserRole).toString();
    
    // Parse to set current verse for navigation
    QString book;
    int chapter, startVerse, endVerse;
    if (bibleManager->parseReference(reference, book, chapter, startVerse, endVerse)) {
        currentBook = book;
        currentChapter = chapter;
        currentVerse = startVerse;
        
        projectButton->setEnabled(true);
        nextButton->setEnabled(true);
        previousButton->setEnabled(currentVerse > 1);
    }
}

void BiblePanel::onProjectClicked()
{
    // Get the currently selected verse from the list
    QListWidgetItem *item = resultsList->currentItem();
    if (!item) return;
    
    QString reference = item->data(Qt::UserRole).toString();
    
    // Find the verse in currentVerses
    for (const BibleVerse &verse : currentVerses) {
        if (verse.reference() == reference) {
            emit verseSelected(reference, verse.text);
            return;
        }
    }
}

void BiblePanel::onNextVerse()
{
    int currentRow = resultsList->currentRow();
    if (currentRow < resultsList->count() - 1) {
        resultsList->setCurrentRow(currentRow + 1);
        onProjectClicked();  // Project the next verses
    }
}

void BiblePanel::onPreviousVerse()
{
    int currentRow = resultsList->currentRow();
    if (currentRow > 0) {
        resultsList->setCurrentRow(currentRow - 1);
        onProjectClicked();  // Project the previous verses
    }
}

void BiblePanel::onBibleLoaded(const QString &translation)
{
    referenceSearchEdit->setEnabled(true);
    textSearchEdit->setEnabled(true);
    resultsList->setEnabled(true);
    
    // Update autocomplete for book names every time Bible loads (translation change)
    QStringList bookNames = bibleManager->getBookNames();
    if (!bookCompleter) {
        bookCompleter = new QCompleter(bookNames, this);
        bookCompleter->setCaseSensitivity(Qt::CaseInsensitive);
        bookCompleter->setCompletionMode(QCompleter::PopupCompletion);
        referenceSearchEdit->setCompleter(bookCompleter);
        
        // Add space after completion and move cursor to end
        connect(bookCompleter, QOverload<const QString &>::of(&QCompleter::activated),
                this, [this](const QString &text) {
                    // Block signals to prevent triggering search
                    referenceSearchEdit->blockSignals(true);
                    referenceSearchEdit->setText(text + " ");
                    referenceSearchEdit->setCursorPosition(text.length() + 1);
                    referenceSearchEdit->blockSignals(false);
                    referenceSearchEdit->setFocus();
                });
    } else {
        // Update existing completer with new book names
        bookCompleter->setModel(new QStringListModel(bookNames, bookCompleter));
    }
}

void BiblePanel::onTranslationChanged(int index)
{
    
    QString biblePath = translationCombo->itemData(index).toString();
    defaultBiblePath = biblePath;
    bibleManager->loadBible(biblePath);
}

void BiblePanel::refreshAvailableBibles()
{
    QSettings settings("SimplePresenter", "SimplePresenter");
    settings.beginGroup("ProjectionCanvas");
    defaultBiblePath = settings.value("defaultBibleTranslationPath").toString();
    settings.endGroup();

    loadBibles();
}

void BiblePanel::parseAndDisplayVerse(const QString &reference)
{
    QString book;
    int chapter, startVerse, endVerse;
    
    if (bibleManager->parseReference(reference, book, chapter, startVerse, endVerse)) {
        // Get all verses from startVerse to the end of the chapter
        currentVerses = bibleManager->getVerses(book, chapter, startVerse, 999); // 999 = to end of chapter
        
        if (!currentVerses.isEmpty()) {
            currentBook = book;
            currentChapter = chapter;
            currentVerse = startVerse;
            
            // Display all following verses in the results list
            displayVerses(currentVerses);
            
            projectButton->setEnabled(true);
            nextButton->setEnabled(true);
            previousButton->setEnabled(currentVerse > 1);
        }
    }
}

void BiblePanel::displayVerses(const QVector<BibleVerse> &verses)
{
    resultsList->clear();
    
    for (const BibleVerse &verse : verses) {
        QString displayText = QString("%1 - %2").arg(verse.reference()).arg(verse.text);
        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::UserRole, verse.reference());
        item->setToolTip(verse.text);
        resultsList->addItem(item);

        QWidget *rowWidget = new QWidget(resultsList);
        QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(4, 0, 4, 0);
        QToolButton *addButton = new QToolButton(rowWidget);
        addButton->setText("+");
        addButton->setFixedWidth(24);
        addButton->setAutoRaise(true);
        addButton->setToolTip("Add to playlist");
        QLabel *label = new QLabel(displayText, rowWidget);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        // Place the '+' button on the left, verse text on the right.
        rowLayout->addWidget(addButton);
        rowLayout->addWidget(label);
        rowWidget->setLayout(rowLayout);
        resultsList->setItemWidget(item, rowWidget);

        connect(addButton, &QToolButton::clicked, this, [this, reference = verse.reference()]() {
            emit addVerseToPlaylist(reference);
        });
    }
    
    // Don't auto-select when displaying - let user click
    if (!verses.isEmpty()) {
        resultsList->setCurrentRow(0);
    }
}

void BiblePanel::onContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem *item = resultsList->itemAt(pos);
    if (!item) return;
    
    QMenu contextMenu(this);
    QAction *addToPlaylistAction = contextMenu.addAction("Add to Playlist");
    QAction *projectAction = contextMenu.addAction("Project");
    
    QAction *selectedAction = contextMenu.exec(resultsList->mapToGlobal(pos));
    
    if (selectedAction == addToPlaylistAction) {
        QString reference = item->data(Qt::UserRole).toString();
        emit addVerseToPlaylist(reference);
    } else if (selectedAction == projectAction) {
        onSearchResultClicked(item);
        onProjectClicked();
    }
}
