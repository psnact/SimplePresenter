#include "SongEditorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QXmlStreamWriter>

SongEditorDialog::SongEditorDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Add New Song");
    setupUI();
    resize(800, 600);
}

SongEditorDialog::SongEditorDialog(const Song &song, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Song");
    setupUI();
    loadSong(song);
    resize(800, 600);
}

void SongEditorDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Song info group
    QGroupBox *infoGroup = new QGroupBox("Song Information");
    QFormLayout *infoForm = new QFormLayout(infoGroup);
    
    titleEdit = new QLineEdit();
    infoForm->addRow("Title:", titleEdit);
    
    authorEdit = new QLineEdit();
    infoForm->addRow("Author (optional):", authorEdit);
    
    copyrightEdit = new QLineEdit();
    infoForm->addRow("Copyright (optional):", copyrightEdit);
    
    ccliEdit = new QLineEdit();
    infoForm->addRow("CCLI (optional):", ccliEdit);
    
    mainLayout->addWidget(infoGroup);
    
    // Lyrics editor
    QGroupBox *lyricsGroup = new QGroupBox("Song Lyrics");
    QVBoxLayout *lyricsLayout = new QVBoxLayout(lyricsGroup);
    
    QLabel *instructionLabel = new QLabel(
        "Paste your song lyrics below. Separate sections with a blank line.\n"
        "Each section will be displayed separately when projecting."
    );
    instructionLabel->setWordWrap(true);
    lyricsLayout->addWidget(instructionLabel);
    
    sectionTextEdit = new QTextEdit();
    sectionTextEdit->setPlaceholderText(
        "Example:\n\n"
        "First verse line 1\n"
        "First verse line 2\n"
        "\n"
        "Chorus line 1\n"
        "Chorus line 2\n"
        "\n"
        "Second verse line 1\n"
        "Second verse line 2"
    );
    lyricsLayout->addWidget(sectionTextEdit);
    
    mainLayout->addWidget(lyricsGroup);
    
    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    mainLayout->addWidget(buttonBox);
    
    // Connect signals
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &SongEditorDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &SongEditorDialog::onRejected);
}

void SongEditorDialog::loadSong(const Song &song)
{
    currentSong = song;
    
    titleEdit->setText(song.title);
    authorEdit->setText(song.author);
    copyrightEdit->setText(song.copyright);
    ccliEdit->setText(song.ccli);
    
    // Convert sections back to text format
    QString allText;
    for (const SongSection &section : song.sections) {
        if (!allText.isEmpty()) {
            allText += "\n\n";  // Blank line between sections
        }
        allText += section.lines.join("\n");
    }
    sectionTextEdit->setPlainText(allText);
}

void SongEditorDialog::onAccepted()
{
    // Validate
    if (titleEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Missing Title", "Please enter a song title.");
        return;
    }
    
    QString lyricsText = sectionTextEdit->toPlainText().trimmed();
    if (lyricsText.isEmpty()) {
        QMessageBox::warning(this, "No Lyrics", "Please enter song lyrics.");
        return;
    }
    
    // Parse lyrics into sections (separated by blank lines)
    currentSong.sections.clear();
    QStringList paragraphs = lyricsText.split("\n\n", Qt::SkipEmptyParts);
    
    for (int i = 0; i < paragraphs.size(); ++i) {
        QString paragraph = paragraphs[i].trimmed();
        if (!paragraph.isEmpty()) {
            SongSection section;
            section.index = i;
            section.type = "verse";  // Default type
            section.lines = paragraph.split("\n");
            currentSong.sections.append(section);
        }
    }
    
    if (currentSong.sections.isEmpty()) {
        QMessageBox::warning(this, "No Sections", "Could not parse any sections from the lyrics.");
        return;
    }
    
    // Update song info
    currentSong.title = titleEdit->text().trimmed();
    currentSong.author = authorEdit->text().trimmed();
    currentSong.copyright = copyrightEdit->text().trimmed();
    currentSong.ccli = ccliEdit->text().trimmed();
    
    accept();
}

void SongEditorDialog::onRejected()
{
    reject();
}

Song SongEditorDialog::getSong() const
{
    return currentSong;
}
