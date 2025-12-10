#include "SettingsDialog.h"
#include "BibleManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSettings>
#include <QFileDialog>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QApplication>
#include <QClipboard>
#include <QScrollArea>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QSignalBlocker>
#include <QFileInfo>
#include <functional>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , projectionTextColor(Qt::white)
    , projectionBgColor(0, 0, 0, 180)
    , projectionRefHighlightColor(0, 0, 0, 180)
    , projectionBgOpacity(70)
    , projectionBgCornerRadius(0)
    , projectionRefHighlightOpacity(70)
    , projectionRefHighlightCornerRadius(0)
    , songTextColor(Qt::white)
    , songBgColor(0, 0, 0, 180)
    , songBorderColor(Qt::white)
    , songBgOpacity(70)
    , songBgCornerRadius(0)
    , songLineSpacingPercent(120)
    , songBorderEnabled(false)
    , songBorderThickness(4)
    , songBorderPaddingHorizontal(20)
    , songBorderPaddingVertical(20)
    , defaultBibleTranslationPath()
    , verseBackgroundColor(Qt::black)
    , songBackgroundColor(Qt::black)
    , obsTextColor(Qt::white)
    , obsRefColor(Qt::white)
    , obsTextHighlightColor(0, 0, 0, 180)
    , obsRefHighlightColor(0, 0, 0, 180)
{
    setWindowTitle("Settings");
    resize(600, 500);
    
    setupUI();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

namespace {

static void setComboToValue(QComboBox *combo, const QString &value)
{
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemData(i).toString() == value) {
            combo->setCurrentIndex(i);
            return;
        }
    }
    combo->setCurrentIndex(0);
}

static void setButtonColorPreview(QPushButton *button, const QColor &color)
{
    if (!button) {
        return;
    }

    QColor displayColor = color;
    if (!displayColor.isValid()) {
        displayColor = QColor(Qt::transparent);
    }

    const double luminance = (0.299 * displayColor.red() + 0.587 * displayColor.green() + 0.114 * displayColor.blue()) / 255.0;
    QColor textColor = luminance > 0.6 ? QColor(Qt::black) : QColor(Qt::white);

    QString style = QString("QPushButton { background-color: rgba(%1, %2, %3, %4); color: %5; }")
                        .arg(displayColor.red())
                        .arg(displayColor.green())
                        .arg(displayColor.blue())
                        .arg(displayColor.alpha())
                        .arg(textColor.name());

    button->setStyleSheet(style);
}

static void showGroupInDialog(QGroupBox *group, QWidget *owner, const QString &title)
{
    if (!group || !owner) {
        return;
    }

    QDialog dialog(owner);
    dialog.setWindowTitle(title);
    dialog.setMinimumWidth(520);
    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);

    group->setVisible(true);
    group->setParent(scrollWidget);
    scrollLayout->addWidget(group);

    scrollArea->setWidget(scrollWidget);
    dialogLayout->addWidget(scrollArea);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    dialogLayout->addWidget(buttonBox);

    dialog.exec();

    group->setParent(owner);
    group->setVisible(false);
}

static void syncFontButtonsWithFont(const QFont &font,
                                    QToolButton *boldButton,
                                    QToolButton *italicButton,
                                    QToolButton *underlineButton)
{
    if (boldButton) {
        QSignalBlocker blocker(boldButton);
        boldButton->setChecked(font.bold());
    }
    if (italicButton) {
        QSignalBlocker blocker(italicButton);
        italicButton->setChecked(font.italic());
    }
    if (underlineButton) {
        QSignalBlocker blocker(underlineButton);
        underlineButton->setChecked(font.underline());
    }
}

static QFont fontFromControls(const QFontComboBox *combo,
                              const QSpinBox *sizeSpin,
                              const QToolButton *boldButton,
                              const QToolButton *italicButton,
                              const QToolButton *underlineButton)
{
    QFont font = combo ? combo->currentFont() : QFont();
    if (sizeSpin && sizeSpin->value() > 0) {
        font.setPointSize(sizeSpin->value());
    }
    if (boldButton) {
        font.setBold(boldButton->isChecked());
    }
    if (italicButton) {
        font.setItalic(italicButton->isChecked());
    }
    if (underlineButton) {
        font.setUnderline(underlineButton->isChecked());
    }
    return font;
}

} // namespace

void SettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    tabWidget = new QTabWidget();
    
    // Projection canvas tab
    QWidget *projectionTab = new QWidget();
    QVBoxLayout *projectionLayout = new QVBoxLayout(projectionTab);
    
    // Display/Monitor Selection
    QGroupBox *displayGroup = new QGroupBox("Display Selection");
    QFormLayout *displayForm = new QFormLayout(displayGroup);
    
    displayCombo = new QComboBox();
    QList<QScreen*> screens = QGuiApplication::screens();
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    
    for (int i = 0; i < screens.size(); ++i) {
        QScreen *screen = screens[i];
        QString displayName = QString("Display %1").arg(i + 1);
        
        if (screen == primaryScreen) {
            displayName += " (Primary - Current App)";
        }
        
        QRect geometry = screen->geometry();
        displayName += QString(" - %1x%2").arg(geometry.width()).arg(geometry.height());
        
        displayCombo->addItem(displayName, i);
    }
    
    displayForm->addRow("Projection Display:", displayCombo);
    
    QLabel *displayNote = new QLabel("Select which monitor/display to show the projection canvas in fullscreen.");
    displayNote->setWordWrap(true);
    displayNote->setStyleSheet("color: gray; font-style: italic;");
    displayForm->addRow("", displayNote);
    
    projectionLayout->addWidget(displayGroup);
    
    songLyricsGroup = new QGroupBox("Song Lyrics Settings", this);
    songLyricsGroup->setVisible(false);
    QFormLayout *songLyricsForm = new QFormLayout(songLyricsGroup);
    songFontCombo = new QFontComboBox();
    QWidget *songFontWidget = new QWidget(this);
    QHBoxLayout *songFontLayout = new QHBoxLayout(songFontWidget);
    songFontLayout->setContentsMargins(0, 0, 0, 0);
    songFontLayout->setSpacing(4);
    songFontLayout->addWidget(songFontCombo, 1);

    auto configureFormatButton = [](QToolButton *button, const QString &text, const QString &tooltip, std::function<void(QFont&)> fontAdjuster) {
        button->setText(text);
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setToolTip(tooltip);
        button->setFocusPolicy(Qt::NoFocus);
        button->setFixedWidth(28);
        QFont btnFont = button->font();
        fontAdjuster(btnFont);
        button->setFont(btnFont);
    };

    songFontBoldButton = new QToolButton(songFontWidget);
    configureFormatButton(songFontBoldButton, "B", "Toggle bold", [](QFont &f) { f.setBold(true); });
    songFontLayout->addWidget(songFontBoldButton);
    connect(songFontBoldButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = songFontCombo->currentFont();
        font.setBold(checked);
        songFontCombo->setCurrentFont(font);
    });

    songFontItalicButton = new QToolButton(songFontWidget);
    configureFormatButton(songFontItalicButton, "I", "Toggle italic", [](QFont &f) { f.setItalic(true); });
    songFontLayout->addWidget(songFontItalicButton);
    connect(songFontItalicButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = songFontCombo->currentFont();
        font.setItalic(checked);
        songFontCombo->setCurrentFont(font);
    });

    songFontUnderlineButton = new QToolButton(songFontWidget);
    configureFormatButton(songFontUnderlineButton, "U", "Toggle underline", [](QFont &f) { f.setUnderline(true); });
    songFontLayout->addWidget(songFontUnderlineButton);
    connect(songFontUnderlineButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = songFontCombo->currentFont();
        font.setUnderline(checked);
        songFontCombo->setCurrentFont(font);
    });
    songFontUppercaseButton = new QToolButton(songFontWidget);
    configureFormatButton(songFontUppercaseButton, "AA", "Toggle ALL CAPS", [](QFont &f) { f.setCapitalization(QFont::AllUppercase); });
    songFontLayout->addWidget(songFontUppercaseButton);
    connect(songFontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        syncFontButtonsWithFont(font, songFontBoldButton, songFontItalicButton, songFontUnderlineButton);
    });

    songLyricsForm->addRow("Font:", songFontWidget);
    songFontSizeSpinBox = new QSpinBox();
    songFontSizeSpinBox->setRange(12, 200);
    songFontSizeSpinBox->setValue(48);
    songLyricsForm->addRow("Font Size:", songFontSizeSpinBox);

    songTextColorButton = new QPushButton("Choose Color");
    songLyricsForm->addRow("Text Color:", songTextColorButton);

    QHBoxLayout *songBgColorLayout = new QHBoxLayout();
    songBgColorButton = new QPushButton("Choose Color");
    songBgColorLayout->addWidget(songBgColorButton);
    songBgColorLayout->addStretch();
    songLyricsForm->addRow("Background Color:", songBgColorLayout);

    songBgOpacitySlider = new QSlider(Qt::Horizontal);
    songBgOpacitySlider->setRange(0, 100);
    songBgOpacitySlider->setSingleStep(5);
    songBgOpacitySlider->setPageStep(10);
    songBgOpacitySlider->setValue(songBgOpacity);
    songBgOpacitySlider->setMinimumWidth(150);
    songBgOpacityValueLabel = new QSpinBox();
    songBgOpacityValueLabel->setRange(0, 100);
    songBgOpacityValueLabel->setSingleStep(songBgOpacitySlider->singleStep());
    songBgOpacityValueLabel->setValue(songBgOpacity);
    songBgOpacityValueLabel->setSuffix("%");
    songBgOpacityValueLabel->setMaximumWidth(70);
    QHBoxLayout *songOpacityLayout = new QHBoxLayout();
    songOpacityLayout->addWidget(songBgOpacitySlider, 1);
    songOpacityLayout->addWidget(songBgOpacityValueLabel);
    songLyricsForm->addRow("Highlight Opacity:", songOpacityLayout);

    songBgCornerRadiusSlider = new QSlider(Qt::Horizontal);
    songBgCornerRadiusSlider->setRange(0, 100);
    songBgCornerRadiusSlider->setSingleStep(1);
    songBgCornerRadiusSlider->setPageStep(5);
    songBgCornerRadiusSlider->setValue(songBgCornerRadius);
    songBgCornerRadiusSlider->setMinimumWidth(150);
    songBgCornerRadiusValueLabel = new QSpinBox();
    songBgCornerRadiusValueLabel->setRange(0, 100);
    songBgCornerRadiusValueLabel->setSingleStep(songBgCornerRadiusSlider->singleStep());
    songBgCornerRadiusValueLabel->setValue(songBgCornerRadius);
    songBgCornerRadiusValueLabel->setSuffix(" px");
    songBgCornerRadiusValueLabel->setMaximumWidth(80);
    QHBoxLayout *songCornerLayout = new QHBoxLayout();
    songCornerLayout->addWidget(songBgCornerRadiusSlider, 1);
    songCornerLayout->addWidget(songBgCornerRadiusValueLabel);
    songLyricsForm->addRow("Corner Radius:", songCornerLayout);

    songLineSpacingSlider = new QSlider(Qt::Horizontal);
    songLineSpacingSlider->setRange(100, 200);
    songLineSpacingSlider->setSingleStep(5);
    songLineSpacingSlider->setPageStep(10);
    songLineSpacingSlider->setValue(songLineSpacingPercent);
    songLineSpacingSlider->setMinimumWidth(150);
    songLineSpacingValueLabel = new QSpinBox();
    songLineSpacingValueLabel->setRange(100, 200);
    songLineSpacingValueLabel->setSingleStep(songLineSpacingSlider->singleStep());
    songLineSpacingValueLabel->setValue(songLineSpacingPercent);
    songLineSpacingValueLabel->setSuffix("%");
    songLineSpacingValueLabel->setMaximumWidth(80);
    QHBoxLayout *songLineSpacingLayout = new QHBoxLayout();
    songLineSpacingLayout->addWidget(songLineSpacingSlider, 1);
    songLineSpacingLayout->addWidget(songLineSpacingValueLabel);
    songLyricsForm->addRow("Line Spacing:", songLineSpacingLayout);

    songBorderEnabledCheck = new QCheckBox("Draw border around highlight");
    songBorderEnabledCheck->setChecked(songBorderEnabled);
    songLyricsForm->addRow("Border:", songBorderEnabledCheck);

    songBorderColorButton = new QPushButton("Choose Border Color");
    songLyricsForm->addRow("Border Color:", songBorderColorButton);

    songBorderThicknessSlider = new QSlider(Qt::Horizontal);
    songBorderThicknessSlider->setRange(1, 20);
    songBorderThicknessSlider->setSingleStep(1);
    songBorderThicknessSlider->setPageStep(2);
    songBorderThicknessSlider->setValue(songBorderThickness);
    songBorderThicknessSlider->setMinimumWidth(150);
    songBorderThicknessValueLabel = new QSpinBox();
    songBorderThicknessValueLabel->setRange(1, 20);
    songBorderThicknessValueLabel->setSingleStep(songBorderThicknessSlider->singleStep());
    songBorderThicknessValueLabel->setValue(songBorderThickness);
    songBorderThicknessValueLabel->setSuffix(" px");
    songBorderThicknessValueLabel->setMaximumWidth(80);
    QHBoxLayout *songBorderLayout = new QHBoxLayout();
    songBorderLayout->addWidget(songBorderThicknessSlider, 1);
    songBorderLayout->addWidget(songBorderThicknessValueLabel);
    songLyricsForm->addRow("Border Thickness:", songBorderLayout);

    songBorderPaddingHorizontalSlider = new QSlider(Qt::Horizontal);
    songBorderPaddingHorizontalSlider->setRange(0, 400);
    songBorderPaddingHorizontalSlider->setSingleStep(1);
    songBorderPaddingHorizontalSlider->setPageStep(5);
    songBorderPaddingHorizontalSlider->setValue(songBorderPaddingHorizontal);
    songBorderPaddingHorizontalSlider->setMinimumWidth(150);
    songBorderPaddingHorizontalValueLabel = new QSpinBox();
    songBorderPaddingHorizontalValueLabel->setRange(0, 400);
    songBorderPaddingHorizontalValueLabel->setSingleStep(songBorderPaddingHorizontalSlider->singleStep());
    songBorderPaddingHorizontalValueLabel->setValue(songBorderPaddingHorizontal);
    songBorderPaddingHorizontalValueLabel->setSuffix(" px");
    songBorderPaddingHorizontalValueLabel->setMaximumWidth(90);
    QHBoxLayout *songBorderHPaddingLayout = new QHBoxLayout();
    songBorderHPaddingLayout->addWidget(songBorderPaddingHorizontalSlider, 1);
    songBorderHPaddingLayout->addWidget(songBorderPaddingHorizontalValueLabel);
    songLyricsForm->addRow("Border Horizontal Padding:", songBorderHPaddingLayout);

    songBorderPaddingVerticalSlider = new QSlider(Qt::Horizontal);
    songBorderPaddingVerticalSlider->setRange(0, 400);
    songBorderPaddingVerticalSlider->setSingleStep(1);
    songBorderPaddingVerticalSlider->setPageStep(5);
    songBorderPaddingVerticalSlider->setValue(songBorderPaddingVertical);
    songBorderPaddingVerticalSlider->setMinimumWidth(150);
    songBorderPaddingVerticalValueLabel = new QSpinBox();
    songBorderPaddingVerticalValueLabel->setRange(0, 400);
    songBorderPaddingVerticalValueLabel->setSingleStep(songBorderPaddingVerticalSlider->singleStep());
    songBorderPaddingVerticalValueLabel->setValue(songBorderPaddingVertical);
    songBorderPaddingVerticalValueLabel->setSuffix(" px");
    songBorderPaddingVerticalValueLabel->setMaximumWidth(90);
    QHBoxLayout *songBorderVPaddingLayout = new QHBoxLayout();
    songBorderVPaddingLayout->addWidget(songBorderPaddingVerticalSlider, 1);
    songBorderVPaddingLayout->addWidget(songBorderPaddingVerticalValueLabel);
    songLyricsForm->addRow("Border Vertical Padding:", songBorderVPaddingLayout);

    updateSongBorderControls();

    songAlignmentCombo = new QComboBox();
    songAlignmentCombo->addItem("Left", Qt::AlignLeft);
    songAlignmentCombo->addItem("Center", Qt::AlignCenter);
    songAlignmentCombo->addItem("Right", Qt::AlignRight);
    songAlignmentCombo->setCurrentIndex(1);
    songLyricsForm->addRow("Text Alignment:", songAlignmentCombo);

    songPositionCombo = new QComboBox();
    songPositionCombo->addItem("Top", 0);
    songPositionCombo->addItem("Center", 1);
    songPositionCombo->addItem("Bottom", 2);
    songPositionCombo->setCurrentIndex(1);
    songLyricsForm->addRow("Vertical Position:", songPositionCombo);

    QGroupBox *songLyricsLauncher = new QGroupBox("Song Lyrics");
    QVBoxLayout *songLyricsLauncherLayout = new QVBoxLayout(songLyricsLauncher);
    QPushButton *songLyricsButton = new QPushButton("Song Lyrics Settings...");
    songLyricsButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel *songLyricsNote = new QLabel("Configure fonts, colors, and highlight options for song lyrics.");
    songLyricsNote->setWordWrap(true);
    songLyricsLauncherLayout->addWidget(songLyricsButton);
    songLyricsLauncherLayout->addWidget(songLyricsNote);
    projectionLayout->addWidget(songLyricsLauncher);

    bibleTextGroup = new QGroupBox("Bible Text Settings", this);
    bibleTextGroup->setVisible(false);
    QFormLayout *bibleForm = new QFormLayout(bibleTextGroup);
    
    projectionFontCombo = new QFontComboBox();
    QWidget *bibleFontWidget = new QWidget(this);
    QHBoxLayout *bibleFontLayout = new QHBoxLayout(bibleFontWidget);
    bibleFontLayout->setContentsMargins(0, 0, 0, 0);
    bibleFontLayout->setSpacing(4);
    bibleFontLayout->addWidget(projectionFontCombo, 1);

    projectionFontBoldButton = new QToolButton(bibleFontWidget);
    configureFormatButton(projectionFontBoldButton, "B", "Toggle bold", [](QFont &f) { f.setBold(true); });
    bibleFontLayout->addWidget(projectionFontBoldButton);
    connect(projectionFontBoldButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = projectionFontCombo->currentFont();
        font.setBold(checked);
        projectionFontCombo->setCurrentFont(font);
    });

    projectionFontItalicButton = new QToolButton(bibleFontWidget);
    configureFormatButton(projectionFontItalicButton, "I", "Toggle italic", [](QFont &f) { f.setItalic(true); });
    bibleFontLayout->addWidget(projectionFontItalicButton);
    connect(projectionFontItalicButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = projectionFontCombo->currentFont();
        font.setItalic(checked);
        projectionFontCombo->setCurrentFont(font);
    });

    projectionFontUnderlineButton = new QToolButton(bibleFontWidget);
    configureFormatButton(projectionFontUnderlineButton, "U", "Toggle underline", [](QFont &f) { f.setUnderline(true); });
    bibleFontLayout->addWidget(projectionFontUnderlineButton);
    connect(projectionFontUnderlineButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = projectionFontCombo->currentFont();
        font.setUnderline(checked);
        projectionFontCombo->setCurrentFont(font);
    });
    projectionFontUppercaseButton = new QToolButton(bibleFontWidget);
    configureFormatButton(projectionFontUppercaseButton, "AA", "Toggle ALL CAPS", [](QFont &f) { f.setCapitalization(QFont::AllUppercase); });
    bibleFontLayout->addWidget(projectionFontUppercaseButton);
    connect(projectionFontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        syncFontButtonsWithFont(font, projectionFontBoldButton, projectionFontItalicButton, projectionFontUnderlineButton);
    });

    bibleForm->addRow("Font:", bibleFontWidget);
    
    projectionFontSizeSpinBox = new QSpinBox();
    projectionFontSizeSpinBox->setRange(12, 200);
    projectionFontSizeSpinBox->setValue(48);
    bibleForm->addRow("Font Size:", projectionFontSizeSpinBox);
    
    projectionTextColorButton = new QPushButton("Choose Color");
    bibleForm->addRow("Text Color:", projectionTextColorButton);

    QHBoxLayout *projBgColorLayout = new QHBoxLayout();
    projectionBgColorButton = new QPushButton("Choose Color");
    projBgColorLayout->addWidget(projectionBgColorButton);
    projBgColorLayout->addStretch();
    bibleForm->addRow("Background Color:", projBgColorLayout);

    projectionBgOpacitySlider = new QSlider(Qt::Horizontal);
    projectionBgOpacitySlider->setRange(0, 100);
    projectionBgOpacitySlider->setSingleStep(5);
    projectionBgOpacitySlider->setPageStep(10);
    projectionBgOpacitySlider->setValue(projectionBgOpacity);
    projectionBgOpacitySlider->setMinimumWidth(150);
    projectionBgOpacityValueLabel = new QSpinBox();
    projectionBgOpacityValueLabel->setRange(0, 100);
    projectionBgOpacityValueLabel->setSingleStep(projectionBgOpacitySlider->singleStep());
    projectionBgOpacityValueLabel->setValue(projectionBgOpacity);
    projectionBgOpacityValueLabel->setSuffix("%");
    projectionBgOpacityValueLabel->setMaximumWidth(70);
    QHBoxLayout *projBgOpacityLayout = new QHBoxLayout();
    projBgOpacityLayout->addWidget(projectionBgOpacitySlider, 1);
    projBgOpacityLayout->addWidget(projectionBgOpacityValueLabel);
    bibleForm->addRow("Highlight Opacity:", projBgOpacityLayout);

    QHBoxLayout *projCornerLayout = new QHBoxLayout();
    projectionBgCornerRadiusSlider = new QSlider(Qt::Horizontal);
    projectionBgCornerRadiusSlider->setRange(0, 100);
    projectionBgCornerRadiusSlider->setSingleStep(1);
    projectionBgCornerRadiusSlider->setPageStep(5);
    projectionBgCornerRadiusSlider->setValue(projectionBgCornerRadius);
    projectionBgCornerRadiusSlider->setMinimumWidth(150);
    projectionBgCornerRadiusValueLabel = new QSpinBox();
    projectionBgCornerRadiusValueLabel->setRange(0, 100);
    projectionBgCornerRadiusValueLabel->setSingleStep(projectionBgCornerRadiusSlider->singleStep());
    projectionBgCornerRadiusValueLabel->setValue(projectionBgCornerRadius);
    projectionBgCornerRadiusValueLabel->setSuffix(" px");
    projectionBgCornerRadiusValueLabel->setMaximumWidth(80);
    projCornerLayout->addWidget(projectionBgCornerRadiusSlider, 1);
    projCornerLayout->addWidget(projectionBgCornerRadiusValueLabel);
    bibleForm->addRow("Corner Radius:", projCornerLayout);
    
    // Text alignment
    projectionAlignmentCombo = new QComboBox();
    projectionAlignmentCombo->addItem("Left", Qt::AlignLeft);
    projectionAlignmentCombo->addItem("Center", Qt::AlignCenter);
    projectionAlignmentCombo->addItem("Right", Qt::AlignRight);
    projectionAlignmentCombo->setCurrentIndex(1); // Center by default
    bibleForm->addRow("Text Alignment:", projectionAlignmentCombo);
    
    // Vertical position
    projectionPositionCombo = new QComboBox();
    projectionPositionCombo->addItem("Top", 0);
    projectionPositionCombo->addItem("Center", 1);
    projectionPositionCombo->addItem("Bottom", 2);
    projectionPositionCombo->setCurrentIndex(1); // Center by default
    bibleForm->addRow("Vertical Position:", projectionPositionCombo);

    defaultBibleVersionCombo = new QComboBox();
    populateDefaultBibleVersions();
    bibleForm->addRow("Default Bible Version:", defaultBibleVersionCombo);
    
    QGroupBox *projectionOverlayLauncher = new QGroupBox("Bible Text");
    QVBoxLayout *projectionOverlayLauncherLayout = new QVBoxLayout(projectionOverlayLauncher);
    QPushButton *bibleTextButton = new QPushButton("Bible Text Settings...");
    bibleTextButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel *bibleTextNote = new QLabel("Configure fonts, colors, alignment, and highlight options for Bible text.");
    bibleTextNote->setWordWrap(true);
    projectionOverlayLauncherLayout->addWidget(bibleTextButton);
    projectionOverlayLauncherLayout->addWidget(bibleTextNote);
    projectionLayout->addWidget(projectionOverlayLauncher);
    
    // Bible Reference formatting
    projectionReferenceGroup = new QGroupBox("Bible Reference Format", this);
    projectionReferenceGroup->setVisible(false);
    QFormLayout *projectionRefForm = new QFormLayout(projectionReferenceGroup);
    
    projectionRefFontCombo = new QFontComboBox();
    QWidget *refFontWidget = new QWidget(this);
    QHBoxLayout *refFontLayout = new QHBoxLayout(refFontWidget);
    refFontLayout->setContentsMargins(0, 0, 0, 0);
    refFontLayout->setSpacing(4);
    refFontLayout->addWidget(projectionRefFontCombo, 1);

    projectionRefFontBoldButton = new QToolButton(refFontWidget);
    configureFormatButton(projectionRefFontBoldButton, "B", "Toggle bold", [](QFont &f) { f.setBold(true); });
    refFontLayout->addWidget(projectionRefFontBoldButton);
    connect(projectionRefFontBoldButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = projectionRefFontCombo->currentFont();
        font.setBold(checked);
        projectionRefFontCombo->setCurrentFont(font);
    });

    projectionRefFontItalicButton = new QToolButton(refFontWidget);
    configureFormatButton(projectionRefFontItalicButton, "I", "Toggle italic", [](QFont &f) { f.setItalic(true); });
    refFontLayout->addWidget(projectionRefFontItalicButton);
    connect(projectionRefFontItalicButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = projectionRefFontCombo->currentFont();
        font.setItalic(checked);
        projectionRefFontCombo->setCurrentFont(font);
    });

    projectionRefFontUnderlineButton = new QToolButton(refFontWidget);
    configureFormatButton(projectionRefFontUnderlineButton, "U", "Toggle underline", [](QFont &f) { f.setUnderline(true); });
    refFontLayout->addWidget(projectionRefFontUnderlineButton);
    connect(projectionRefFontUnderlineButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = projectionRefFontCombo->currentFont();
        font.setUnderline(checked);
        projectionRefFontCombo->setCurrentFont(font);
    });
    projectionRefFontUppercaseButton = new QToolButton(refFontWidget);
    configureFormatButton(projectionRefFontUppercaseButton, "AA", "Toggle ALL CAPS", [](QFont &f) { f.setCapitalization(QFont::AllUppercase); });
    refFontLayout->addWidget(projectionRefFontUppercaseButton);
    connect(projectionRefFontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        syncFontButtonsWithFont(font, projectionRefFontBoldButton, projectionRefFontItalicButton, projectionRefFontUnderlineButton);
    });

    projectionRefForm->addRow("Reference Font:", refFontWidget);
    
    projectionRefFontSizeSpinBox = new QSpinBox();
    projectionRefFontSizeSpinBox->setRange(12, 200);
    projectionRefFontSizeSpinBox->setValue(36);
    projectionRefForm->addRow("Reference Size:", projectionRefFontSizeSpinBox);
    
    projectionRefColorButton = new QPushButton("Choose Color");
    projectionRefForm->addRow("Reference Color:", projectionRefColorButton);

    projectionRefHighlightCheck = new QCheckBox("Highlight background");
    projectionRefHighlightColorButton = new QPushButton("Choose Highlight Color");
    projectionRefHighlightColorButton->setEnabled(false);
    projectionRefHighlightOpacitySlider = new QSlider(Qt::Horizontal);
    projectionRefHighlightOpacitySlider->setRange(0, 100);
    projectionRefHighlightOpacitySlider->setSingleStep(5);
    projectionRefHighlightOpacitySlider->setPageStep(10);
    projectionRefHighlightOpacitySlider->setValue(projectionRefHighlightOpacity);
    projectionRefHighlightOpacitySlider->setEnabled(false);
    projectionRefHighlightOpacitySlider->setMinimumWidth(120);
    projectionRefHighlightOpacityValueLabel = new QSpinBox();
    projectionRefHighlightOpacityValueLabel->setRange(0, 100);
    projectionRefHighlightOpacityValueLabel->setSingleStep(projectionRefHighlightOpacitySlider->singleStep());
    projectionRefHighlightOpacityValueLabel->setValue(projectionRefHighlightOpacity);
    projectionRefHighlightOpacityValueLabel->setSuffix("%");
    projectionRefHighlightOpacityValueLabel->setMaximumWidth(70);
    projectionRefHighlightOpacityValueLabel->setEnabled(false);
    QHBoxLayout *refHighlightLayout = new QHBoxLayout();
    refHighlightLayout->addWidget(projectionRefHighlightCheck);
    refHighlightLayout->addWidget(projectionRefHighlightColorButton);
    refHighlightLayout->addStretch();
    projectionRefForm->addRow("Reference Highlight:", refHighlightLayout);

    QHBoxLayout *refOpacityLayout = new QHBoxLayout();
    refOpacityLayout->addWidget(projectionRefHighlightOpacitySlider, 1);
    refOpacityLayout->addWidget(projectionRefHighlightOpacityValueLabel);
    projectionRefForm->addRow("Highlight Opacity:", refOpacityLayout);
    projectionRefHighlightCornerRadiusSlider = new QSlider(Qt::Horizontal);
    projectionRefHighlightCornerRadiusSlider->setRange(0, 100);
    projectionRefHighlightCornerRadiusSlider->setSingleStep(1);
    projectionRefHighlightCornerRadiusSlider->setPageStep(5);
    projectionRefHighlightCornerRadiusSlider->setValue(projectionRefHighlightCornerRadius);
    projectionRefHighlightCornerRadiusSlider->setEnabled(false);
    projectionRefHighlightCornerRadiusSlider->setMinimumWidth(120);
    projectionRefHighlightCornerRadiusValueLabel = new QSpinBox();
    projectionRefHighlightCornerRadiusValueLabel->setRange(0, 100);
    projectionRefHighlightCornerRadiusValueLabel->setSingleStep(projectionRefHighlightCornerRadiusSlider->singleStep());
    projectionRefHighlightCornerRadiusValueLabel->setValue(projectionRefHighlightCornerRadius);
    projectionRefHighlightCornerRadiusValueLabel->setSuffix(" px");
    projectionRefHighlightCornerRadiusValueLabel->setMaximumWidth(80);
    projectionRefHighlightCornerRadiusValueLabel->setEnabled(false);
    QHBoxLayout *refCornerLayout = new QHBoxLayout();
    refCornerLayout->addWidget(projectionRefHighlightCornerRadiusSlider, 1);
    refCornerLayout->addWidget(projectionRefHighlightCornerRadiusValueLabel);
    projectionRefForm->addRow("Corner Radius:", refCornerLayout);
    
    projectionRefAlignmentCombo = new QComboBox();
    projectionRefAlignmentCombo->addItem("Left", Qt::AlignLeft);
    projectionRefAlignmentCombo->addItem("Center", Qt::AlignCenter);
    projectionRefAlignmentCombo->addItem("Right", Qt::AlignRight);
    projectionRefAlignmentCombo->setCurrentIndex(1); // Center by default
    projectionRefForm->addRow("Reference Alignment:", projectionRefAlignmentCombo);
    
    projectionRefPositionCombo = new QComboBox();
    projectionRefPositionCombo->addItem("Above Main Text", 0);
    projectionRefPositionCombo->addItem("Below Main Text", 1);
    projectionRefPositionCombo->setCurrentIndex(0); // Above by default
    projectionRefForm->addRow("Reference Position:", projectionRefPositionCombo);

    projectionSeparateReferenceCheck = new QCheckBox("Place reference in separate bottom area");
    projectionRefForm->addRow("Separate Reference Area:", projectionSeparateReferenceCheck);

    projectionDisplayVersionCheck = new QCheckBox("Display Bible Version beside reference");
    projectionRefForm->addRow("Display Bible Version:", projectionDisplayVersionCheck);
    
    QGroupBox *projectionReferenceLauncher = new QGroupBox("Bible Reference Format");
    QVBoxLayout *projectionReferenceLauncherLayout = new QVBoxLayout(projectionReferenceLauncher);
    QPushButton *projectionReferenceButton = new QPushButton("Bible Reference Settings...");
    projectionReferenceButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel *projectionReferenceNote = new QLabel("Configure fonts, colors, highlight, and positioning for Bible references.");
    projectionReferenceNote->setWordWrap(true);
    projectionReferenceLauncherLayout->addWidget(projectionReferenceButton);
    projectionReferenceLauncherLayout->addWidget(projectionReferenceNote);
    projectionLayout->addWidget(projectionReferenceLauncher);
    projectionLayout->addStretch();
    
    tabWidget->addTab(projectionTab, "Projection");
    
    // Backgrounds tab
    QWidget *backgroundsTab = new QWidget();
    QVBoxLayout *backgroundsLayout = new QVBoxLayout(backgroundsTab);
    
    separateBackgroundsCheck = new QCheckBox("Use separate backgrounds");
    backgroundsLayout->addWidget(separateBackgroundsCheck);
    
    QGroupBox *verseGroup = new QGroupBox("Bible Verses Background");
    QFormLayout *verseForm = new QFormLayout(verseGroup);
    
    verseBackgroundTypeCombo = new QComboBox();
    verseBackgroundTypeCombo->addItem("Solid Color", "color");
    verseBackgroundTypeCombo->addItem("Image", "image");
    verseBackgroundTypeCombo->addItem("Video", "video");
    verseForm->addRow("Background Type:", verseBackgroundTypeCombo);
    
    verseBackgroundColorButton = new QPushButton("Choose Color");
    verseForm->addRow("Background Color:", verseBackgroundColorButton);
    
    QHBoxLayout *verseImageLayout = new QHBoxLayout();
    verseBackgroundImageEdit = new QLineEdit();
    verseBrowseImageButton = new QPushButton("Browse...");
    verseImageLayout->addWidget(verseBackgroundImageEdit);
    verseImageLayout->addWidget(verseBrowseImageButton);
    verseForm->addRow("Background Image:", verseImageLayout);
    
    QHBoxLayout *verseVideoLayout = new QHBoxLayout();
    verseBackgroundVideoEdit = new QLineEdit();
    verseBrowseVideoButton = new QPushButton("Browse...");
    verseVideoLoopCheck = new QCheckBox("Loop video");
    verseVideoLayout->addWidget(verseBackgroundVideoEdit);
    verseVideoLayout->addWidget(verseBrowseVideoButton);
    verseVideoLayout->addWidget(verseVideoLoopCheck);
    verseForm->addRow("Background Video:", verseVideoLayout);
    
    backgroundsLayout->addWidget(verseGroup);
    
    songBackgroundGroup = new QGroupBox("Song Lyrics Background");
    QFormLayout *songForm = new QFormLayout(songBackgroundGroup);
    
    songBackgroundTypeCombo = new QComboBox();
    songBackgroundTypeCombo->addItem("Solid Color", "color");
    songBackgroundTypeCombo->addItem("Image", "image");
    songBackgroundTypeCombo->addItem("Video", "video");
    songForm->addRow("Background Type:", songBackgroundTypeCombo);
    
    songBackgroundColorButton = new QPushButton("Choose Color");
    songForm->addRow("Background Color:", songBackgroundColorButton);
    
    QHBoxLayout *songImageLayout = new QHBoxLayout();
    songBackgroundImageEdit = new QLineEdit();
    songBrowseImageButton = new QPushButton("Browse...");
    songImageLayout->addWidget(songBackgroundImageEdit);
    songImageLayout->addWidget(songBrowseImageButton);
    songForm->addRow("Background Image:", songImageLayout);
    
    QHBoxLayout *songVideoLayout = new QHBoxLayout();
    songBackgroundVideoEdit = new QLineEdit();
    songBrowseVideoButton = new QPushButton("Browse...");
    songVideoLoopCheck = new QCheckBox("Loop video");
    songVideoLayout->addWidget(songBackgroundVideoEdit);
    songVideoLayout->addWidget(songBrowseVideoButton);
    songVideoLayout->addWidget(songVideoLoopCheck);
    songForm->addRow("Background Video:", songVideoLayout);

    backgroundsLayout->addWidget(songBackgroundGroup);

    // Notes background (used when Projection Notes are shown)
    notesBackgroundGroup = new QGroupBox("Notes Background");
    QFormLayout *notesForm = new QFormLayout(notesBackgroundGroup);

    notesBackgroundTypeCombo = new QComboBox();
    notesBackgroundTypeCombo->addItem("Solid Color", "color");
    notesBackgroundTypeCombo->addItem("Image", "image");
    notesBackgroundTypeCombo->addItem("Video", "video");
    notesForm->addRow("Background Type:", notesBackgroundTypeCombo);

    notesBackgroundColorButton = new QPushButton("Choose Color");
    notesForm->addRow("Background Color:", notesBackgroundColorButton);

    QHBoxLayout *notesImageLayout = new QHBoxLayout();
    notesBackgroundImageEdit = new QLineEdit();
    notesBrowseImageButton = new QPushButton("Browse...");
    notesImageLayout->addWidget(notesBackgroundImageEdit);
    notesImageLayout->addWidget(notesBrowseImageButton);
    notesForm->addRow("Background Image:", notesImageLayout);

    QHBoxLayout *notesVideoLayout = new QHBoxLayout();
    notesBackgroundVideoEdit = new QLineEdit();
    notesBrowseVideoButton = new QPushButton("Browse...");
    notesVideoLoopCheck = new QCheckBox("Loop video");
    notesVideoLayout->addWidget(notesBackgroundVideoEdit);
    notesVideoLayout->addWidget(notesBrowseVideoButton);
    notesVideoLayout->addWidget(notesVideoLoopCheck);
    notesForm->addRow("Background Video:", notesVideoLayout);

    backgroundsLayout->addWidget(notesBackgroundGroup);
    backgroundsLayout->addStretch();
    
    tabWidget->addTab(backgroundsTab, "Backgrounds");
    
    // OBS Overlay tab
    QWidget *obsTab = new QWidget();
    QVBoxLayout *obsLayout = new QVBoxLayout(obsTab);
    
    // Network URL Section
    QGroupBox *networkGroup = new QGroupBox("OBS Browser Source URL");
    QHBoxLayout *networkLayout = new QHBoxLayout(networkGroup);
    
    // Get local IP address
    QString localIP = "127.0.0.1";
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) {
            localIP = address.toString();
            break;
        }
    }
    
    QLineEdit *networkUrlEdit = new QLineEdit(QString("http://%1:8080/overlay").arg(localIP));
    networkUrlEdit->setReadOnly(true);
    networkUrlEdit->setToolTip("Use this URL in OBS Browser Source to access from any device on the network");
    QPushButton *copyNetworkBtn = new QPushButton("Copy URL");
    connect(copyNetworkBtn, &QPushButton::clicked, [networkUrlEdit, copyNetworkBtn]() {
        QApplication::clipboard()->setText(networkUrlEdit->text());
        copyNetworkBtn->setText("Copied!");
        QTimer::singleShot(2000, [copyNetworkBtn]() {
            copyNetworkBtn->setText("Copy URL");
        });
    });
    networkLayout->addWidget(networkUrlEdit);
    networkLayout->addWidget(copyNetworkBtn);
    
    obsLayout->addWidget(networkGroup);
    
    // Canvas Resolution
    QGroupBox *obsCanvasGroup = new QGroupBox("Canvas Resolution");
    QFormLayout *obsCanvasForm = new QFormLayout(obsCanvasGroup);
    
    obsResolutionCombo = new QComboBox();
    obsResolutionCombo->addItem("1920x1080 (Full HD)", QSize(1920, 1080));
    obsResolutionCombo->addItem("1280x720 (HD)", QSize(1280, 720));
    obsResolutionCombo->addItem("3840x2160 (4K)", QSize(3840, 2160));
    obsResolutionCombo->addItem("2560x1440 (2K)", QSize(2560, 1440));
    obsResolutionCombo->setCurrentIndex(0);
    obsCanvasForm->addRow("Resolution:", obsResolutionCombo);
    
    QLabel *resolutionNote = new QLabel("Note: Changing resolution requires refreshing the OBS Browser Source.");
    resolutionNote->setWordWrap(true);
    resolutionNote->setStyleSheet("color: gray; font-style: italic;");
    obsCanvasForm->addRow("", resolutionNote);
    
    obsLayout->addWidget(obsCanvasGroup);
    
    // Text Formatting Button
    QGroupBox *textFormattingGroup = new QGroupBox("Text Formatting");
    QVBoxLayout *textFormattingLayout = new QVBoxLayout(textFormattingGroup);
    
    QPushButton *textFormattingBtn = new QPushButton("Text Formatting Settings...");
    textFormattingBtn->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(textFormattingBtn, &QPushButton::clicked, this, &SettingsDialog::showTextFormatting);
    textFormattingLayout->addWidget(textFormattingBtn);
    
    QLabel *textNote = new QLabel("Configure fonts, colors, shadows, outlines, and highlights for text.");
    textNote->setWordWrap(true);
    textNote->setStyleSheet("color: gray; font-style: italic;");
    textFormattingLayout->addWidget(textNote);
    
    obsLayout->addWidget(textFormattingGroup);
    
    // Bible Reference Formatting Button
    QGroupBox *refFormattingGroup = new QGroupBox("Bible Reference Formatting");
    QVBoxLayout *refFormattingLayout = new QVBoxLayout(refFormattingGroup);
    
    QPushButton *refFormattingBtn = new QPushButton("Bible Reference Formatting Settings...");
    refFormattingBtn->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(refFormattingBtn, &QPushButton::clicked, this, &SettingsDialog::showReferenceFormatting);
    refFormattingLayout->addWidget(refFormattingBtn);
    
    QLabel *refNote = new QLabel("Configure fonts, colors, shadows, outlines, and highlights for Bible references.");
    refNote->setWordWrap(true);
    refNote->setStyleSheet("color: gray; font-style: italic;");
    refFormattingLayout->addWidget(refNote);
    
    obsLayout->addWidget(refFormattingGroup);
    obsLayout->addStretch();
    
    tabWidget->addTab(obsTab, "OBS Overlay");
    
    // Keep the formatting widgets but hide them (we'll show them in a dialog)
    QGroupBox *obsTextGroup = new QGroupBox("Text Formatting");
    obsTextGroup->setVisible(false);
    QFormLayout *obsTextForm = new QFormLayout(obsTextGroup);
    
    obsTextFontCombo = new QFontComboBox();
    QWidget *obsTextFontWidget = new QWidget(this);
    QHBoxLayout *obsTextFontLayout = new QHBoxLayout(obsTextFontWidget);
    obsTextFontLayout->setContentsMargins(0, 0, 0, 0);
    obsTextFontLayout->setSpacing(4);
    obsTextFontLayout->addWidget(obsTextFontCombo, 1);

    obsTextBoldButton = new QToolButton(obsTextFontWidget);
    configureFormatButton(obsTextBoldButton, "B", "Toggle bold", [](QFont &f) { f.setBold(true); });
    obsTextFontLayout->addWidget(obsTextBoldButton);
    connect(obsTextBoldButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = obsTextFontCombo->currentFont();
        font.setBold(checked);
        obsTextFontCombo->setCurrentFont(font);
    });

    obsTextItalicButton = new QToolButton(obsTextFontWidget);
    configureFormatButton(obsTextItalicButton, "I", "Toggle italic", [](QFont &f) { f.setItalic(true); });
    obsTextFontLayout->addWidget(obsTextItalicButton);
    connect(obsTextItalicButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = obsTextFontCombo->currentFont();
        font.setItalic(checked);
        obsTextFontCombo->setCurrentFont(font);
    });

    obsTextUnderlineButton = new QToolButton(obsTextFontWidget);
    configureFormatButton(obsTextUnderlineButton, "U", "Toggle underline", [](QFont &f) { f.setUnderline(true); });
    obsTextFontLayout->addWidget(obsTextUnderlineButton);
    connect(obsTextUnderlineButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = obsTextFontCombo->currentFont();
        font.setUnderline(checked);
        obsTextFontCombo->setCurrentFont(font);
    });
    obsTextUppercaseButton = new QToolButton(obsTextFontWidget);
    configureFormatButton(obsTextUppercaseButton, "AA", "Toggle ALL CAPS", [](QFont &f) { f.setCapitalization(QFont::AllUppercase); });
    obsTextFontLayout->addWidget(obsTextUppercaseButton);
    connect(obsTextFontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        syncFontButtonsWithFont(font, obsTextBoldButton, obsTextItalicButton, obsTextUnderlineButton);
    });

    obsTextForm->addRow("Font:", obsTextFontWidget);
    
    obsTextFontSizeSpinBox = new QSpinBox();
    obsTextFontSizeSpinBox->setRange(12, 200);
    obsTextFontSizeSpinBox->setValue(42);
    obsTextForm->addRow("Font Size:", obsTextFontSizeSpinBox);
    
    obsTextColorButton = new QPushButton("Choose Color");
    obsTextForm->addRow("Text Color:", obsTextColorButton);
    
    obsTextShadowCheck = new QCheckBox("Text Shadow");
    obsTextShadowCheck->setChecked(true);
    obsTextForm->addRow("", obsTextShadowCheck);
    
    obsTextOutlineCheck = new QCheckBox("Text Outline/Stroke");
    obsTextOutlineCheck->setChecked(false);
    obsTextForm->addRow("", obsTextOutlineCheck);
    
    obsTextOutlineWidthSpinBox = new QSpinBox();
    obsTextOutlineWidthSpinBox->setRange(1, 10);
    obsTextOutlineWidthSpinBox->setValue(2);
    obsTextOutlineWidthSpinBox->setSuffix(" px");
    obsTextForm->addRow("Outline Width:", obsTextOutlineWidthSpinBox);
    
    obsTextOutlineColorButton = new QPushButton("Choose Outline Color");
    obsTextForm->addRow("Outline Color:", obsTextOutlineColorButton);
    
    obsTextHighlightCheck = new QCheckBox("Highlight Background");
    obsTextHighlightCheck->setChecked(false);
    obsTextForm->addRow("", obsTextHighlightCheck);
    
    obsTextHighlightColorButton = new QPushButton("Choose Highlight Color");
    obsTextForm->addRow("Highlight Color:", obsTextHighlightColorButton);
    
    obsTextHighlightOpacitySlider = new QSlider(Qt::Horizontal);
    obsTextHighlightOpacitySlider->setRange(0, 100);
    obsTextHighlightOpacitySlider->setSingleStep(5);
    obsTextHighlightOpacitySlider->setPageStep(10);
    obsTextHighlightOpacitySlider->setValue(obsTextHighlightOpacity);
    obsTextHighlightOpacitySlider->setEnabled(false);
    obsTextHighlightOpacitySlider->setMinimumWidth(150);
    obsTextHighlightOpacityLabel = new QSpinBox();
    obsTextHighlightOpacityLabel->setRange(0, 100);
    obsTextHighlightOpacityLabel->setSingleStep(obsTextHighlightOpacitySlider->singleStep());
    obsTextHighlightOpacityLabel->setValue(obsTextHighlightOpacity);
    obsTextHighlightOpacityLabel->setSuffix("%");
    obsTextHighlightOpacityLabel->setMaximumWidth(70);
    obsTextHighlightOpacityLabel->setEnabled(false);
    QHBoxLayout *obsTextOpacityLayout = new QHBoxLayout();
    obsTextOpacityLayout->addWidget(obsTextHighlightOpacitySlider, 1);
    obsTextOpacityLayout->addWidget(obsTextHighlightOpacityLabel);
    obsTextForm->addRow("Highlight Opacity:", obsTextOpacityLayout);
    
    obsTextHighlightCornerRadiusSlider = new QSlider(Qt::Horizontal);
    obsTextHighlightCornerRadiusSlider->setRange(0, 100);
    obsTextHighlightCornerRadiusSlider->setSingleStep(1);
    obsTextHighlightCornerRadiusSlider->setPageStep(5);
    obsTextHighlightCornerRadiusSlider->setValue(obsTextHighlightCornerRadius);
    obsTextHighlightCornerRadiusSlider->setEnabled(false);
    obsTextHighlightCornerRadiusSlider->setMinimumWidth(150);
    obsTextHighlightCornerRadiusLabel = new QSpinBox();
    obsTextHighlightCornerRadiusLabel->setRange(0, 100);
    obsTextHighlightCornerRadiusLabel->setSingleStep(obsTextHighlightCornerRadiusSlider->singleStep());
    obsTextHighlightCornerRadiusLabel->setValue(obsTextHighlightCornerRadius);
    obsTextHighlightCornerRadiusLabel->setSuffix(" px");
    obsTextHighlightCornerRadiusLabel->setMaximumWidth(80);
    obsTextHighlightCornerRadiusLabel->setEnabled(false);
    QHBoxLayout *obsTextCornerLayout = new QHBoxLayout();
    obsTextCornerLayout->addWidget(obsTextHighlightCornerRadiusSlider, 1);
    obsTextCornerLayout->addWidget(obsTextHighlightCornerRadiusLabel);
    obsTextForm->addRow("Corner Radius:", obsTextCornerLayout);
    
    obsLayout->addWidget(obsTextGroup);
    
    QGroupBox *obsRefGroup = new QGroupBox("Reference Formatting");
    obsRefGroup->setVisible(false);
    QFormLayout *obsRefForm = new QFormLayout(obsRefGroup);
    
    obsRefFontCombo = new QFontComboBox();
    QWidget *obsRefFontWidget = new QWidget(this);
    QHBoxLayout *obsRefFontLayout = new QHBoxLayout(obsRefFontWidget);
    obsRefFontLayout->setContentsMargins(0, 0, 0, 0);
    obsRefFontLayout->setSpacing(4);
    obsRefFontLayout->addWidget(obsRefFontCombo, 1);

    obsRefBoldButton = new QToolButton(obsRefFontWidget);
    configureFormatButton(obsRefBoldButton, "B", "Toggle bold", [](QFont &f) { f.setBold(true); });
    obsRefFontLayout->addWidget(obsRefBoldButton);
    connect(obsRefBoldButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = obsRefFontCombo->currentFont();
        font.setBold(checked);
        obsRefFontCombo->setCurrentFont(font);
    });

    obsRefItalicButton = new QToolButton(obsRefFontWidget);
    configureFormatButton(obsRefItalicButton, "I", "Toggle italic", [](QFont &f) { f.setItalic(true); });
    obsRefFontLayout->addWidget(obsRefItalicButton);
    connect(obsRefItalicButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = obsRefFontCombo->currentFont();
        font.setItalic(checked);
        obsRefFontCombo->setCurrentFont(font);
    });

    obsRefUnderlineButton = new QToolButton(obsRefFontWidget);
    configureFormatButton(obsRefUnderlineButton, "U", "Toggle underline", [](QFont &f) { f.setUnderline(true); });
    obsRefFontLayout->addWidget(obsRefUnderlineButton);
    connect(obsRefUnderlineButton, &QToolButton::toggled, this, [this](bool checked) {
        QFont font = obsRefFontCombo->currentFont();
        font.setUnderline(checked);
        obsRefFontCombo->setCurrentFont(font);
    });
    obsRefUppercaseButton = new QToolButton(obsRefFontWidget);
    configureFormatButton(obsRefUppercaseButton, "AA", "Toggle ALL CAPS", [](QFont &f) { f.setCapitalization(QFont::AllUppercase); });
    obsRefFontLayout->addWidget(obsRefUppercaseButton);
    connect(obsRefFontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        syncFontButtonsWithFont(font, obsRefBoldButton, obsRefItalicButton, obsRefUnderlineButton);
    });

    obsRefForm->addRow("Font:", obsRefFontWidget);
    
    obsRefFontSizeSpinBox = new QSpinBox();
    obsRefFontSizeSpinBox->setRange(12, 200);
    obsRefFontSizeSpinBox->setValue(28);
    obsRefForm->addRow("Font Size:", obsRefFontSizeSpinBox);
    
    obsRefColorButton = new QPushButton("Choose Color");
    obsRefForm->addRow("Reference Color:", obsRefColorButton);
    
    obsRefShadowCheck = new QCheckBox("Text Shadow");
    obsRefShadowCheck->setChecked(true);
    obsRefForm->addRow("", obsRefShadowCheck);
    
    obsRefOutlineCheck = new QCheckBox("Text Outline/Stroke");
    obsRefOutlineCheck->setChecked(false);
    obsRefForm->addRow("", obsRefOutlineCheck);
    
    obsRefOutlineWidthSpinBox = new QSpinBox();
    obsRefOutlineWidthSpinBox->setRange(1, 10);
    obsRefOutlineWidthSpinBox->setValue(2);
    obsRefOutlineWidthSpinBox->setSuffix(" px");
    obsRefForm->addRow("Outline Width:", obsRefOutlineWidthSpinBox);
    
    obsRefOutlineColorButton = new QPushButton("Choose Outline Color");
    obsRefForm->addRow("Outline Color:", obsRefOutlineColorButton);
    
    obsRefHighlightCheck = new QCheckBox("Highlight Background");
    obsRefHighlightCheck->setChecked(false);
    obsRefForm->addRow("", obsRefHighlightCheck);
    
    obsRefHighlightColorButton = new QPushButton("Choose Highlight Color");
    obsRefForm->addRow("Highlight Color:", obsRefHighlightColorButton);
    
    obsRefHighlightOpacitySlider = new QSlider(Qt::Horizontal);
    obsRefHighlightOpacitySlider->setRange(0, 100);
    obsRefHighlightOpacitySlider->setSingleStep(5);
    obsRefHighlightOpacitySlider->setPageStep(10);
    obsRefHighlightOpacitySlider->setValue(obsRefHighlightOpacity);
    obsRefHighlightOpacitySlider->setEnabled(false);
    obsRefHighlightOpacitySlider->setMinimumWidth(150);
    obsRefHighlightOpacityLabel = new QSpinBox();
    obsRefHighlightOpacityLabel->setRange(0, 100);
    obsRefHighlightOpacityLabel->setSingleStep(obsRefHighlightOpacitySlider->singleStep());
    obsRefHighlightOpacityLabel->setValue(obsRefHighlightOpacity);
    obsRefHighlightOpacityLabel->setSuffix("%");
    obsRefHighlightOpacityLabel->setMaximumWidth(70);
    obsRefHighlightOpacityLabel->setEnabled(false);
    QHBoxLayout *obsRefOpacityLayout = new QHBoxLayout();
    obsRefOpacityLayout->addWidget(obsRefHighlightOpacitySlider, 1);
    obsRefOpacityLayout->addWidget(obsRefHighlightOpacityLabel);
    obsRefForm->addRow("Highlight Opacity:", obsRefOpacityLayout);
    
    obsRefHighlightCornerRadiusSlider = new QSlider(Qt::Horizontal);
    obsRefHighlightCornerRadiusSlider->setRange(0, 100);
    obsRefHighlightCornerRadiusSlider->setSingleStep(1);
    obsRefHighlightCornerRadiusSlider->setPageStep(5);
    obsRefHighlightCornerRadiusSlider->setValue(obsRefHighlightCornerRadius);
    obsRefHighlightCornerRadiusSlider->setEnabled(false);
    obsRefHighlightCornerRadiusSlider->setMinimumWidth(150);
    obsRefHighlightCornerRadiusLabel = new QSpinBox();
    obsRefHighlightCornerRadiusLabel->setRange(0, 100);
    obsRefHighlightCornerRadiusLabel->setSingleStep(obsRefHighlightCornerRadiusSlider->singleStep());
    obsRefHighlightCornerRadiusLabel->setValue(obsRefHighlightCornerRadius);
    obsRefHighlightCornerRadiusLabel->setSuffix(" px");
    obsRefHighlightCornerRadiusLabel->setMaximumWidth(80);
    obsRefHighlightCornerRadiusLabel->setEnabled(false);
    QHBoxLayout *obsRefCornerLayout = new QHBoxLayout();
    obsRefCornerLayout->addWidget(obsRefHighlightCornerRadiusSlider, 1);
    obsRefCornerLayout->addWidget(obsRefHighlightCornerRadiusLabel);
    obsRefForm->addRow("Corner Radius:", obsRefCornerLayout);
    
    obsRefDisplayVersionCheck = new QCheckBox("Display Bible Version beside reference");
    obsRefForm->addRow("Display Bible Version:", obsRefDisplayVersionCheck);
    
    obsLayout->addWidget(obsRefGroup);
    
    mainLayout->addWidget(tabWidget);
    
    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    mainLayout->addWidget(buttonBox);
    
    // Connect signals
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::onRejected);
    connect(projectionTextColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onProjectionTextColorClicked);
    connect(projectionRefColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onProjectionRefColorClicked);
    connect(projectionBgColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onProjectionBgColorClicked);
    connect(projectionRefHighlightCheck, &QCheckBox::toggled,
            this, &SettingsDialog::onProjectionRefHighlightToggled);
    connect(bibleTextButton, &QPushButton::clicked,
            this, &SettingsDialog::onBibleTextSettingsClicked);
    connect(songLyricsButton, &QPushButton::clicked,
            this, &SettingsDialog::onSongLyricsSettingsClicked);
    connect(projectionReferenceButton, &QPushButton::clicked,
            this, &SettingsDialog::onProjectionReferenceSettingsClicked);
    connect(projectionRefHighlightColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onProjectionRefHighlightColorClicked);
    connect(projectionRefHighlightOpacitySlider, &QSlider::valueChanged,
            this, &SettingsDialog::onProjectionRefHighlightOpacityChanged);
    connect(projectionRefHighlightCornerRadiusSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onProjectionRefHighlightCornerRadiusChanged);
    connect(projectionBgOpacitySlider, &QSlider::valueChanged,
            this, &SettingsDialog::onProjectionBgOpacityChanged);
    connect(projectionBgCornerRadiusSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onProjectionBgCornerRadiusChanged);
    connect(songTextColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onSongTextColorClicked);
    connect(songBgColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onSongBgColorClicked);
    connect(songBgOpacitySlider, &QSlider::valueChanged,
            this, &SettingsDialog::onSongBgOpacityChanged);
    connect(songBgCornerRadiusSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onSongBgCornerRadiusChanged);
    connect(songLineSpacingSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onSongLineSpacingChanged);
    connect(songBorderEnabledCheck, &QCheckBox::toggled,
            this, &SettingsDialog::onSongBorderToggled);
    connect(songBorderColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onSongBorderColorClicked);
    connect(songBorderThicknessSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onSongBorderThicknessChanged);
    connect(songBorderPaddingHorizontalSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onSongBorderHorizontalPaddingChanged);
    connect(songBorderPaddingVerticalSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onSongBorderVerticalPaddingChanged);
    connect(defaultBibleVersionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onDefaultBibleVersionChanged);
    connect(separateBackgroundsCheck, &QCheckBox::toggled,
            this, &SettingsDialog::onSeparateBackgroundsToggled);
    connect(verseBackgroundColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onVerseBackgroundColorClicked);
    connect(songBackgroundColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onSongBackgroundColorClicked);
    connect(obsTextColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onOBSTextColorClicked);
    connect(obsRefColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onOBSRefColorClicked);
    connect(obsTextHighlightColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onOBSTextHighlightColorClicked);
    connect(obsTextHighlightCheck, &QCheckBox::toggled,
            this, &SettingsDialog::onOBSTextHighlightToggled);
    connect(obsTextHighlightOpacitySlider, &QSlider::valueChanged,
            this, &SettingsDialog::onOBSTextHighlightOpacityChanged);
    connect(obsTextHighlightCornerRadiusSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onOBSTextHighlightCornerRadiusChanged);
    connect(obsRefHighlightColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onOBSRefHighlightColorClicked);
    connect(obsRefHighlightCheck, &QCheckBox::toggled,
            this, &SettingsDialog::onOBSRefHighlightToggled);
    connect(obsRefHighlightOpacitySlider, &QSlider::valueChanged,
            this, &SettingsDialog::onOBSRefHighlightOpacityChanged);
    connect(obsRefHighlightCornerRadiusSlider, &QSlider::valueChanged,
            this, &SettingsDialog::onOBSRefHighlightCornerRadiusChanged);
    connect(obsTextOutlineColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onOBSTextOutlineColorClicked);
    connect(obsRefOutlineColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onOBSRefOutlineColorClicked);
    connect(verseBrowseImageButton, &QPushButton::clicked,
            this, &SettingsDialog::onBrowseVerseBackgroundImage);
    connect(verseBrowseVideoButton, &QPushButton::clicked,
            this, &SettingsDialog::onBrowseVerseBackgroundVideo);
    connect(songBrowseImageButton, &QPushButton::clicked,
            this, &SettingsDialog::onBrowseSongBackgroundImage);
    connect(songBrowseVideoButton, &QPushButton::clicked,
            this, &SettingsDialog::onBrowseSongBackgroundVideo);

    connect(verseBackgroundTypeCombo, &QComboBox::currentIndexChanged,
            this, [this](int){ if (!separateBackgroundsCheck->isChecked()) syncSongBackgroundToVerse(); });
    connect(verseBackgroundImageEdit, &QLineEdit::textChanged,
            this, [this](const QString &){ if (!separateBackgroundsCheck->isChecked()) syncSongBackgroundToVerse(); });
    connect(verseBackgroundVideoEdit, &QLineEdit::textChanged,
            this, [this](const QString &){ if (!separateBackgroundsCheck->isChecked()) syncSongBackgroundToVerse(); });
    connect(verseVideoLoopCheck, &QCheckBox::toggled,
            this, [this](bool){ if (!separateBackgroundsCheck->isChecked()) syncSongBackgroundToVerse(); });

    connect(notesBackgroundColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onNotesBackgroundColorClicked);
    connect(notesBrowseImageButton, &QPushButton::clicked,
            this, &SettingsDialog::onBrowseNotesBackgroundImage);
    connect(notesBrowseVideoButton, &QPushButton::clicked,
            this, &SettingsDialog::onBrowseNotesBackgroundVideo);
}

void SettingsDialog::useSeparateBackgrounds(bool separate)
{
    songBackgroundGroup->setEnabled(separate);
    if (notesBackgroundGroup) {
        notesBackgroundGroup->setEnabled(separate);
    }

    if (!separate) {
        syncSongBackgroundToVerse();
    }
}

void SettingsDialog::syncSongBackgroundToVerse()
{
    songBackgroundTypeCombo->setCurrentIndex(verseBackgroundTypeCombo->currentIndex());
    songBackgroundColor = verseBackgroundColor;
    songBackgroundImageEdit->setText(verseBackgroundImageEdit->text());
    songBackgroundVideoEdit->setText(verseBackgroundVideoEdit->text());
    songVideoLoopCheck->setChecked(verseVideoLoopCheck->isChecked());
}

void SettingsDialog::updateSongBorderControls()
{
    bool allowBorder = (songLineSpacingPercent == 100);
    songBorderEnabledCheck->setEnabled(allowBorder);
    if (!allowBorder && songBorderEnabled) {
        songBorderEnabled = false;
        QSignalBlocker blocker(songBorderEnabledCheck);
        songBorderEnabledCheck->setChecked(false);
    }

    bool enableBorderControls = allowBorder && songBorderEnabled;
    songBorderThicknessSlider->setEnabled(enableBorderControls);
    songBorderThicknessValueLabel->setEnabled(enableBorderControls);
    songBorderPaddingHorizontalSlider->setEnabled(enableBorderControls);
    songBorderPaddingHorizontalValueLabel->setEnabled(enableBorderControls);
    songBorderPaddingVerticalSlider->setEnabled(enableBorderControls);
    songBorderPaddingVerticalValueLabel->setEnabled(enableBorderControls);
    songBorderColorButton->setEnabled(enableBorderControls);
    setButtonColorPreview(songBorderColorButton, songBorderColor);
}

void SettingsDialog::populateDefaultBibleVersions()
{
    if (!defaultBibleVersionCombo) {
        return;
    }

    QSignalBlocker blocker(defaultBibleVersionCombo);
    defaultBibleVersionCombo->clear();
    defaultBibleVersionCombo->addItem("Automatic (first available)", QString());

    BibleManager bibleManager;
    const QStringList availableBibles = bibleManager.getAvailableBibles();
    for (const QString &path : availableBibles) {
        QFileInfo info(path);
        QString label = info.baseName();
        if (label.isEmpty()) {
            label = path;
        }
        defaultBibleVersionCombo->addItem(label, path);
    }

    defaultBibleVersionCombo->setEnabled(defaultBibleVersionCombo->count() > 1);

    int index = defaultBibleVersionCombo->findData(defaultBibleTranslationPath);
    if (index < 0) {
        index = 0;
    }
    defaultBibleVersionCombo->setCurrentIndex(index);
}

void SettingsDialog::loadSettings()
{
    QSettings settings("SimplePresenter", "SimplePresenter");
    
    // Projection settings
    settings.beginGroup("ProjectionCanvas");
    
    // Load display selection
    int savedDisplay = settings.value("displayIndex", 0).toInt();
    if (savedDisplay < displayCombo->count()) {
        displayCombo->setCurrentIndex(savedDisplay);
    }
    
    QFont projFont = settings.value("font", QFont("Arial", 48)).value<QFont>();
    if (projFont.pointSize() <= 0) {
        projFont.setPointSize(48);
    }
    projectionFontCombo->setCurrentFont(projFont);
    projectionFontSizeSpinBox->setValue(projFont.pointSize());
    syncFontButtonsWithFont(projFont, projectionFontBoldButton, projectionFontItalicButton, projectionFontUnderlineButton);
    bool projUppercase = settings.value("textUppercase", false).toBool();
    projectionFontUppercaseButton->setChecked(projUppercase);

    QFont projRefFont = settings.value("referenceFont", QFont("Arial", 36)).value<QFont>();
    if (projRefFont.pointSize() <= 0) {
        projRefFont.setPointSize(36);
    }
    projectionRefFontCombo->setCurrentFont(projRefFont);
    projectionRefFontSizeSpinBox->setValue(projRefFont.pointSize());
    syncFontButtonsWithFont(projRefFont, projectionRefFontBoldButton, projectionRefFontItalicButton, projectionRefFontUnderlineButton);
    bool projRefUppercase = settings.value("referenceUppercase", false).toBool();
    projectionRefFontUppercaseButton->setChecked(projRefUppercase);
    
    projectionTextColor = settings.value("textColor", QColor(Qt::white)).value<QColor>();
    projectionRefColor = settings.value("referenceColor", QColor(Qt::white)).value<QColor>();
    projectionBgColor = settings.value("backgroundColor", QColor(0, 0, 0, 180)).value<QColor>();
    projectionRefHighlightColor = settings.value("referenceHighlightColor", projectionRefHighlightColor).value<QColor>();
    bool refHighlightEnabled = settings.value("referenceHighlightEnabled", false).toBool();
    int storedOpacity = settings.value("backgroundOpacity", -1).toInt();
    if (storedOpacity >= 0) {
        projectionBgOpacity = qBound(0, storedOpacity, 100);
    } else {
        bool transparentBg = settings.value("backgroundTransparent", false).toBool();
        if (transparentBg) {
            projectionBgOpacity = 0;
        } else {
            projectionBgOpacity = qRound(projectionBgColor.alpha() * 100.0 / 255.0);
        }
    }
    projectionBgColor.setAlpha(qRound(projectionBgOpacity * 255.0 / 100.0));
    projectionBgCornerRadius = settings.value("backgroundCornerRadius", projectionBgCornerRadius).toInt();
    projectionBgCornerRadius = qBound(0, projectionBgCornerRadius, 200);
    {
        QSignalBlocker blocker(projectionBgOpacitySlider);
        projectionBgOpacitySlider->setValue(projectionBgOpacity);
    }
    projectionBgOpacityValueLabel->setValue(projectionBgOpacity);
    connect(projectionBgOpacityValueLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            projectionBgOpacitySlider, &QSlider::setValue);
    {
        QSignalBlocker blocker(projectionBgCornerRadiusSlider);
        projectionBgCornerRadiusSlider->setValue(projectionBgCornerRadius);
    }
    projectionBgCornerRadiusValueLabel->setValue(projectionBgCornerRadius);
    connect(projectionBgCornerRadiusValueLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            projectionBgCornerRadiusSlider, &QSlider::setValue);
    projectionRefHighlightOpacity = qRound(projectionRefHighlightColor.alpha() * 100.0 / 255.0);
    int storedRefOpacity = settings.value("referenceHighlightOpacity", -1).toInt();
    if (storedRefOpacity >= 0) {
        projectionRefHighlightOpacity = qBound(0, storedRefOpacity, 100);
    }
    projectionRefHighlightColor.setAlpha(qRound(projectionRefHighlightOpacity * 255.0 / 100.0));
    projectionRefHighlightCornerRadius = settings.value("referenceHighlightCornerRadius", projectionRefHighlightCornerRadius).toInt();
    projectionRefHighlightCornerRadius = qBound(0, projectionRefHighlightCornerRadius, 200);
    {
        QSignalBlocker blocker(projectionRefHighlightCheck);
        projectionRefHighlightCheck->setChecked(refHighlightEnabled);
    }
    projectionRefHighlightColorButton->setEnabled(refHighlightEnabled);
    {
        QSignalBlocker blocker(projectionRefHighlightOpacitySlider);
        projectionRefHighlightOpacitySlider->setValue(projectionRefHighlightOpacity);
    }
    projectionRefHighlightOpacitySlider->setEnabled(refHighlightEnabled);
    projectionRefHighlightOpacityValueLabel->setEnabled(refHighlightEnabled);
    projectionRefHighlightOpacityValueLabel->setValue(projectionRefHighlightOpacity);
    connect(projectionRefHighlightOpacityValueLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            projectionRefHighlightOpacitySlider, &QSlider::setValue);
    projectionRefHighlightCornerRadiusSlider->setEnabled(refHighlightEnabled);
    projectionRefHighlightCornerRadiusValueLabel->setEnabled(refHighlightEnabled);
    projectionRefHighlightCornerRadiusSlider->setValue(projectionRefHighlightCornerRadius);
    projectionRefHighlightCornerRadiusValueLabel->setValue(projectionRefHighlightCornerRadius);
    connect(projectionRefHighlightCornerRadiusValueLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            projectionRefHighlightCornerRadiusSlider, &QSlider::setValue);
    setButtonColorPreview(projectionTextColorButton, projectionTextColor);
    setButtonColorPreview(projectionRefColorButton, projectionRefColor);
    setButtonColorPreview(projectionBgColorButton, projectionBgColor);
    setButtonColorPreview(projectionRefHighlightColorButton, projectionRefHighlightColor);
    
    int projAlignment = settings.value("alignment", Qt::AlignCenter).toInt();
    for (int i = 0; i < projectionAlignmentCombo->count(); ++i) {
        if (projectionAlignmentCombo->itemData(i).toInt() == projAlignment) {
            projectionAlignmentCombo->setCurrentIndex(i);
            break;
        }
    }
    
    int projRefAlignment = settings.value("referenceAlignment", Qt::AlignCenter).toInt();
    for (int i = 0; i < projectionRefAlignmentCombo->count(); ++i) {
        if (projectionRefAlignmentCombo->itemData(i).toInt() == projRefAlignment) {
            projectionRefAlignmentCombo->setCurrentIndex(i);
            break;
        }
    }
    
    int projRefPosition = settings.value("referencePosition", 0).toInt();
    projectionRefPositionCombo->setCurrentIndex(projRefPosition);
    bool separateReference = settings.value("separateReferenceArea", false).toBool();
    projectionSeparateReferenceCheck->setChecked(separateReference);

    bool projDisplayVersion = settings.value("displayVersion", false).toBool();
    projectionDisplayVersionCheck->setChecked(projDisplayVersion);
    
    int projPosition = settings.value("verticalPosition", 1).toInt();
    projectionPositionCombo->setCurrentIndex(projPosition);

    // Song lyrics settings
    QFont defaultSongFont("Arial", 48);
    QFont songFont = settings.value("song/font", defaultSongFont).value<QFont>();
    if (songFont.pointSize() <= 0) {
        songFont.setPointSize(defaultSongFont.pointSize());
    }
    songFontCombo->setCurrentFont(songFont);
    songFontSizeSpinBox->setValue(songFont.pointSize());
    syncFontButtonsWithFont(songFont, songFontBoldButton, songFontItalicButton, songFontUnderlineButton);
    bool songUppercase = settings.value("song/textUppercase", false).toBool();
    songFontUppercaseButton->setChecked(songUppercase);

    songTextColor = settings.value("song/textColor", songTextColor).value<QColor>();
    songBgColor = settings.value("song/backgroundColor", songBgColor).value<QColor>();
    int storedSongOpacity = settings.value("song/backgroundOpacity", songBgOpacity).toInt();
    songBgOpacity = qBound(0, storedSongOpacity, 100);
    songBgColor.setAlpha(qRound(songBgOpacity * 255.0 / 100.0));
    songBgCornerRadius = qBound(0, settings.value("song/backgroundCornerRadius", songBgCornerRadius).toInt(), 200);
    songLineSpacingPercent = qBound(100, settings.value("song/lineSpacingPercent", songLineSpacingPercent).toInt(), 200);
    {
        QSignalBlocker blocker(songBgOpacitySlider);
        songBgOpacitySlider->setValue(songBgOpacity);
    }
    songBgOpacityValueLabel->setValue(songBgOpacity);
    connect(songBgOpacityValueLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            songBgOpacitySlider, &QSlider::setValue);
    {
        QSignalBlocker blocker(songBgCornerRadiusSlider);
        songBgCornerRadiusSlider->setValue(songBgCornerRadius);
    }
    songBgCornerRadiusValueLabel->setValue(songBgCornerRadius);
    connect(songBgCornerRadiusValueLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            songBgCornerRadiusSlider, &QSlider::setValue);
    {
        QSignalBlocker blocker(songLineSpacingSlider);
        songLineSpacingSlider->setValue(songLineSpacingPercent);
    }
    songLineSpacingValueLabel->setValue(songLineSpacingPercent);
    connect(songLineSpacingValueLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            songLineSpacingSlider, &QSlider::setValue);
    defaultBibleTranslationPath = settings.value("defaultBibleTranslationPath", defaultBibleTranslationPath).toString();
    if (defaultBibleVersionCombo) {
        int defaultIndex = defaultBibleVersionCombo->findData(defaultBibleTranslationPath);
        if (defaultIndex < 0) {
            defaultIndex = 0;
        }
        {
            QSignalBlocker blocker(defaultBibleVersionCombo);
            defaultBibleVersionCombo->setCurrentIndex(defaultIndex);
        }
        defaultBibleTranslationPath = defaultBibleVersionCombo->currentData().toString();
    }
    songBorderEnabled = settings.value("song/borderEnabled", songBorderEnabled).toBool();
    songBorderThickness = qBound(1, settings.value("song/borderThickness", songBorderThickness).toInt(), 20);
    songBorderColor = settings.value("song/borderColor", songBorderColor).value<QColor>();
    songBorderPaddingHorizontal = qBound(0, settings.value("song/borderPaddingHorizontal", songBorderPaddingHorizontal).toInt(), 400);
    songBorderPaddingVertical = qBound(0, settings.value("song/borderPaddingVertical", songBorderPaddingVertical).toInt(), 400);
    songBorderEnabledCheck->setChecked(songBorderEnabled);
    {
        QSignalBlocker blocker(songBorderThicknessSlider);
        songBorderThicknessSlider->setValue(songBorderThickness);
    }
    songBorderThicknessValueLabel->setValue(songBorderThickness);
    connect(songBorderThicknessValueLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            songBorderThicknessSlider, &QSlider::setValue);
    {
        QSignalBlocker blocker(songBorderPaddingHorizontalSlider);
        songBorderPaddingHorizontalSlider->setValue(songBorderPaddingHorizontal);
    }
    songBorderPaddingHorizontalValueLabel->setValue(songBorderPaddingHorizontal);
    connect(songBorderPaddingHorizontalValueLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            songBorderPaddingHorizontalSlider, &QSlider::setValue);
    {
        QSignalBlocker blocker(songBorderPaddingVerticalSlider);
        songBorderPaddingVerticalSlider->setValue(songBorderPaddingVertical);
    }
    songBorderPaddingVerticalValueLabel->setValue(songBorderPaddingVertical);
    connect(songBorderPaddingVerticalValueLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            songBorderPaddingVerticalSlider, &QSlider::setValue);
    updateSongBorderControls();
    setButtonColorPreview(songTextColorButton, songTextColor);
    setButtonColorPreview(songBgColorButton, songBgColor);

    int songAlignment = settings.value("song/alignment", Qt::AlignCenter).toInt();
    for (int i = 0; i < songAlignmentCombo->count(); ++i) {
        if (songAlignmentCombo->itemData(i).toInt() == songAlignment) {
            songAlignmentCombo->setCurrentIndex(i);
            break;
        }
    }

    int songPosition = settings.value("song/verticalPosition", 1).toInt();
    for (int i = 0; i < songPositionCombo->count(); ++i) {
        if (songPositionCombo->itemData(i).toInt() == songPosition) {
            songPositionCombo->setCurrentIndex(i);
            break;
        }
    }
    
    settings.endGroup();
    
    // Background settings
    settings.beginGroup("Backgrounds");
    bool hasVerseKeys = settings.contains("verse/backgroundType") || settings.contains("verse/backgroundColor") ||
                        settings.contains("verse/image") || settings.contains("verse/video") ||
                        settings.contains("verse/loop");

    QString verseType;
    QColor verseColor;
    QString verseImage;
    QString verseVideo;
    bool verseLoop;

    if (hasVerseKeys) {
        verseType = settings.value("verse/backgroundType", "color").toString();
        verseColor = settings.value("verse/backgroundColor", QColor(Qt::black)).value<QColor>();
        verseImage = settings.value("verse/image", "").toString();
        verseVideo = settings.value("verse/video", "").toString();
        verseLoop = settings.value("verse/loop", true).toBool();
    } else {
        verseType = settings.value("backgroundType", "color").toString();
        verseColor = settings.value("backgroundColor", QColor(Qt::black)).value<QColor>();
        verseImage = settings.value("defaultImage", "").toString();
        verseVideo = settings.value("defaultVideo", "").toString();
        verseLoop = settings.value("backgroundVideoLoop", true).toBool();
    }

    verseBackgroundColor = verseColor;
    setComboToValue(verseBackgroundTypeCombo, verseType);
    verseBackgroundImageEdit->setText(verseImage);
    verseBackgroundVideoEdit->setText(verseVideo);
    verseVideoLoopCheck->setChecked(verseLoop);

    bool separate = hasVerseKeys && settings.value("separateSongBackground", false).toBool();
    separateBackgroundsCheck->setChecked(separate);
    useSeparateBackgrounds(separate);

    if (separate) {
        QString songType = settings.value("song/backgroundType", verseType).toString();
        songBackgroundColor = settings.value("song/backgroundColor", verseBackgroundColor).value<QColor>();
        songBackgroundImageEdit->setText(settings.value("song/image", "").toString());
        songBackgroundVideoEdit->setText(settings.value("song/video", "").toString());
        songVideoLoopCheck->setChecked(settings.value("song/loop", verseLoop).toBool());
        setComboToValue(songBackgroundTypeCombo, songType);
    } else {
        syncSongBackgroundToVerse();
        songVideoLoopCheck->setChecked(verseVideoLoopCheck->isChecked());
    }

    // Notes background (optional). If not present, default to solid color.
    QString notesType = settings.value("notes/backgroundType", QStringLiteral("color")).toString();
    notesBackgroundColor = settings.value("notes/backgroundColor", verseBackgroundColor).value<QColor>();
    notesBackgroundImageEdit->setText(settings.value("notes/image", "").toString());
    notesBackgroundVideoEdit->setText(settings.value("notes/video", "").toString());
    notesVideoLoopCheck->setChecked(settings.value("notes/loop", verseLoop).toBool());
    setComboToValue(notesBackgroundTypeCombo, notesType);

    settings.endGroup();
    
    // OBS Overlay settings
    settings.beginGroup("OBSOverlay");
    
    // Load resolution
    int resWidth = settings.value("canvasWidth", 1920).toInt();
    int resHeight = settings.value("canvasHeight", 1080).toInt();
    for (int i = 0; i < obsResolutionCombo->count(); ++i) {
        QSize size = obsResolutionCombo->itemData(i).toSize();
        if (size.width() == resWidth && size.height() == resHeight) {
            obsResolutionCombo->setCurrentIndex(i);
            break;
        }
    }
    
    QString obsTextFontFamily = settings.value("textFont", "Arial").toString();
    int obsTextPointSize = settings.value("textFontSize", 42).toInt();
    bool obsTextBold = settings.value("textBold", true).toBool();
    bool obsTextItalic = settings.value("textItalic", false).toBool();
    bool obsTextUnderline = settings.value("textUnderline", false).toBool();
    bool obsTextUppercase = settings.value("textUppercase", false).toBool();
    QFont obsTextFont(obsTextFontFamily, obsTextPointSize);
    obsTextFont.setBold(obsTextBold);
    obsTextFont.setItalic(obsTextItalic);
    obsTextFont.setUnderline(obsTextUnderline);
    obsTextFontCombo->setCurrentFont(obsTextFont);
    obsTextFontSizeSpinBox->setValue(obsTextPointSize);
    syncFontButtonsWithFont(obsTextFont, obsTextBoldButton, obsTextItalicButton, obsTextUnderlineButton);
    obsTextUppercaseButton->setChecked(obsTextUppercase);
    obsTextColor = settings.value("textColor", QColor(Qt::white)).value<QColor>();
    setButtonColorPreview(obsTextColorButton, obsTextColor);
    obsTextShadowCheck->setChecked(settings.value("textShadow", true).toBool());
    obsTextOutlineCheck->setChecked(settings.value("textOutline", false).toBool());
    obsTextOutlineWidthSpinBox->setValue(settings.value("textOutlineWidth", 2).toInt());
    obsTextOutlineColor = settings.value("textOutlineColor", QColor(Qt::black)).value<QColor>();
    setButtonColorPreview(obsTextOutlineColorButton, obsTextOutlineColor);
    obsTextHighlightCheck->setChecked(settings.value("textHighlight", false).toBool());
    obsTextHighlightColor = settings.value("textHighlightColor", QColor(0, 0, 0, 180)).value<QColor>();
    setButtonColorPreview(obsTextHighlightColorButton, obsTextHighlightColor);
    obsTextHighlightOpacity = qBound(0, settings.value("textHighlightOpacity", qRound(obsTextHighlightColor.alpha() * 100.0 / 255.0)).toInt(), 100);
    {
        QSignalBlocker blocker(obsTextHighlightOpacitySlider);
        obsTextHighlightOpacitySlider->setValue(obsTextHighlightOpacity);
    }
    obsTextHighlightOpacitySlider->setEnabled(obsTextHighlightCheck->isChecked());
    obsTextHighlightOpacityLabel->setEnabled(obsTextHighlightCheck->isChecked());
    obsTextHighlightOpacityLabel->setValue(obsTextHighlightOpacity);
    connect(obsTextHighlightOpacityLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            obsTextHighlightOpacitySlider, &QSlider::setValue);
    obsTextHighlightCornerRadius = qBound(0, settings.value("textHighlightCornerRadius", 0).toInt(), 200);
    {
        QSignalBlocker blocker(obsTextHighlightCornerRadiusSlider);
        obsTextHighlightCornerRadiusSlider->setValue(obsTextHighlightCornerRadius);
    }
    obsTextHighlightCornerRadiusSlider->setEnabled(obsTextHighlightCheck->isChecked());
    obsTextHighlightCornerRadiusLabel->setEnabled(obsTextHighlightCheck->isChecked());
    obsTextHighlightCornerRadiusLabel->setValue(obsTextHighlightCornerRadius);
    connect(obsTextHighlightCornerRadiusLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            obsTextHighlightCornerRadiusSlider, &QSlider::setValue);
    QString obsRefFontFamily = settings.value("refFont", "Arial").toString();
    int obsRefPointSize = settings.value("refFontSize", 28).toInt();
    bool obsRefBold = settings.value("refBold", false).toBool();
    bool obsRefItalic = settings.value("refItalic", true).toBool();
    bool obsRefUnderline = settings.value("refUnderline", false).toBool();
    bool obsRefUppercase = settings.value("refUppercase", false).toBool();
    QFont obsRefFont(obsRefFontFamily, obsRefPointSize);
    obsRefFont.setBold(obsRefBold);
    obsRefFont.setItalic(obsRefItalic);
    obsRefFont.setUnderline(obsRefUnderline);
    obsRefFontCombo->setCurrentFont(obsRefFont);
    obsRefFontSizeSpinBox->setValue(obsRefPointSize);
    syncFontButtonsWithFont(obsRefFont, obsRefBoldButton, obsRefItalicButton, obsRefUnderlineButton);
    obsRefUppercaseButton->setChecked(obsRefUppercase);
    obsRefColor = settings.value("refColor", QColor(Qt::white)).value<QColor>();
    setButtonColorPreview(obsRefColorButton, obsRefColor);
    obsRefShadowCheck->setChecked(settings.value("refShadow", true).toBool());
    obsRefOutlineCheck->setChecked(settings.value("refOutline", false).toBool());
    obsRefOutlineWidthSpinBox->setValue(settings.value("refOutlineWidth", 2).toInt());
    obsRefOutlineColor = settings.value("refOutlineColor", QColor(Qt::black)).value<QColor>();
    setButtonColorPreview(obsRefOutlineColorButton, obsRefOutlineColor);
    obsRefHighlightCheck->setChecked(settings.value("refHighlight", false).toBool());
    obsRefHighlightColor = settings.value("refHighlightColor", QColor(0, 0, 0, 180)).value<QColor>();
    setButtonColorPreview(obsRefHighlightColorButton, obsRefHighlightColor);
    obsRefHighlightOpacity = qBound(0, settings.value("refHighlightOpacity", qRound(obsRefHighlightColor.alpha() * 100.0 / 255.0)).toInt(), 100);
    {
        QSignalBlocker blocker(obsRefHighlightOpacitySlider);
        obsRefHighlightOpacitySlider->setValue(obsRefHighlightOpacity);
    }
    obsRefHighlightOpacitySlider->setEnabled(obsRefHighlightCheck->isChecked());
    obsRefHighlightOpacityLabel->setEnabled(obsRefHighlightCheck->isChecked());
    obsRefHighlightOpacityLabel->setValue(obsRefHighlightOpacity);
    connect(obsRefHighlightOpacityLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            obsRefHighlightOpacitySlider, &QSlider::setValue);
    obsRefHighlightCornerRadius = qBound(0, settings.value("refHighlightCornerRadius", 0).toInt(), 200);
    {
        QSignalBlocker blocker(obsRefHighlightCornerRadiusSlider);
        obsRefHighlightCornerRadiusSlider->setValue(obsRefHighlightCornerRadius);
    }
    obsRefHighlightCornerRadiusSlider->setEnabled(obsRefHighlightCheck->isChecked());
    obsRefHighlightCornerRadiusLabel->setEnabled(obsRefHighlightCheck->isChecked());
    obsRefHighlightCornerRadiusLabel->setValue(obsRefHighlightCornerRadius);
    connect(obsRefHighlightCornerRadiusLabel, QOverload<int>::of(&QSpinBox::valueChanged),
            obsRefHighlightCornerRadiusSlider, &QSlider::setValue);

    bool obsRefDisplayVersion = settings.value("refDisplayVersion", false).toBool();
    obsRefDisplayVersionCheck->setChecked(obsRefDisplayVersion);

    settings.endGroup();
}

void SettingsDialog::saveSettings()
{
    QSettings settings("SimplePresenter", "SimplePresenter");
    settings.beginGroup("ProjectionCanvas");

    settings.setValue("displayIndex", displayCombo->currentData().toInt());

    QFont projFont = fontFromControls(projectionFontCombo,
                                     projectionFontSizeSpinBox,
                                     projectionFontBoldButton,
                                     projectionFontItalicButton,
                                     projectionFontUnderlineButton);
    settings.setValue("font", projFont);

    QFont projRefFont = fontFromControls(projectionRefFontCombo,
                                         projectionRefFontSizeSpinBox,
                                         projectionRefFontBoldButton,
                                         projectionRefFontItalicButton,
                                         projectionRefFontUnderlineButton);
    settings.setValue("referenceFont", projRefFont);

    settings.setValue("textUppercase", projectionFontUppercaseButton->isChecked());
    settings.setValue("referenceUppercase", projectionRefFontUppercaseButton->isChecked());

    settings.setValue("textColor", projectionTextColor);
    settings.setValue("referenceColor", projectionRefColor);

    QColor bgColor = projectionBgColor;
    bgColor.setAlpha(qRound(projectionBgOpacity * 255.0 / 100.0));
    settings.setValue("backgroundColor", bgColor);
    settings.setValue("backgroundOpacity", projectionBgOpacity);
    settings.setValue("backgroundTransparent", projectionBgOpacity == 0);
    settings.setValue("backgroundCornerRadius", projectionBgCornerRadius);

    settings.setValue("textHighlightEnabled", projectionBgOpacity > 0);
    settings.setValue("textHighlightColor", bgColor);
    settings.setValue("textHighlightCornerRadius", projectionBgCornerRadius);

    settings.setValue("referenceHighlightEnabled", projectionRefHighlightCheck->isChecked());
    settings.setValue("referenceHighlightColor", projectionRefHighlightColor);
    settings.setValue("referenceHighlightCornerRadius", projectionRefHighlightCornerRadius);

    settings.setValue("alignment", projectionAlignmentCombo->currentData().toInt());
    settings.setValue("referenceAlignment", projectionRefAlignmentCombo->currentData().toInt());
    settings.setValue("referencePosition", projectionRefPositionCombo->currentData().toInt());
    settings.setValue("separateReferenceArea", projectionSeparateReferenceCheck->isChecked());
    settings.setValue("displayVersion", projectionDisplayVersionCheck->isChecked());
    settings.setValue("verticalPosition", projectionPositionCombo->currentData().toInt());

    QFont songFontToSave = fontFromControls(songFontCombo,
                                            songFontSizeSpinBox,
                                            songFontBoldButton,
                                            songFontItalicButton,
                                            songFontUnderlineButton);
    settings.setValue("song/font", songFontToSave);
    settings.setValue("song/textUppercase", songFontUppercaseButton->isChecked());
    settings.setValue("song/textColor", songTextColor);
    QColor songBgToSave = songBgColor;
    songBgToSave.setAlpha(qRound(songBgOpacity * 255.0 / 100.0));
    settings.setValue("song/backgroundColor", songBgToSave);
    settings.setValue("song/backgroundOpacity", songBgOpacity);
    settings.setValue("song/backgroundCornerRadius", songBgCornerRadius);
    settings.setValue("song/textHighlightEnabled", songBgOpacity > 0);
    settings.setValue("song/textHighlightColor", songBgToSave);
    settings.setValue("song/alignment", songAlignmentCombo->currentData().toInt());
    settings.setValue("song/verticalPosition", songPositionCombo->currentData().toInt());
    settings.setValue("song/lineSpacingPercent", songLineSpacingPercent);
    settings.setValue("song/borderEnabled", songBorderEnabled);
    settings.setValue("song/borderThickness", songBorderThickness);
    settings.setValue("song/borderColor", songBorderColor);
    settings.setValue("song/borderPaddingHorizontal", songBorderPaddingHorizontal);
    settings.setValue("song/borderPaddingVertical", songBorderPaddingVertical);
    settings.setValue("defaultBibleTranslationPath", defaultBibleTranslationPath);
    settings.endGroup();

    settings.beginGroup("Backgrounds");
    settings.setValue("separateSongBackground", separateBackgroundsCheck->isChecked());

    settings.setValue("verse/backgroundType", verseBackgroundTypeCombo->currentData().toString());
    settings.setValue("verse/backgroundColor", verseBackgroundColor);
    settings.setValue("verse/image", verseBackgroundImageEdit->text());
    settings.setValue("verse/video", verseBackgroundVideoEdit->text());
    settings.setValue("verse/loop", verseVideoLoopCheck->isChecked());

    if (separateBackgroundsCheck->isChecked()) {
        settings.setValue("song/backgroundType", songBackgroundTypeCombo->currentData().toString());
        settings.setValue("song/backgroundColor", songBackgroundColor);
        settings.setValue("song/image", songBackgroundImageEdit->text());
        settings.setValue("song/video", songBackgroundVideoEdit->text());
        settings.setValue("song/loop", songVideoLoopCheck->isChecked());
    } else {
        settings.remove(QStringLiteral("song"));
    }

    // Persist notes background configuration
    settings.setValue("notes/backgroundType", notesBackgroundTypeCombo->currentData().toString());
    settings.setValue("notes/backgroundColor", notesBackgroundColor);
    settings.setValue("notes/image", notesBackgroundImageEdit->text());
    settings.setValue("notes/video", notesBackgroundVideoEdit->text());
    settings.setValue("notes/loop", notesVideoLoopCheck->isChecked());

    settings.endGroup();

    settings.beginGroup("OBSOverlay");
    QSize resolution = obsResolutionCombo->currentData().toSize();
    settings.setValue("canvasWidth", resolution.width());
    settings.setValue("canvasHeight", resolution.height());

    QFont obsTextFontToSave = fontFromControls(obsTextFontCombo,
                                              obsTextFontSizeSpinBox,
                                              obsTextBoldButton,
                                              obsTextItalicButton,
                                              obsTextUnderlineButton);
    settings.setValue("textFont", obsTextFontToSave.family());
    settings.setValue("textFontSize", obsTextFontToSave.pointSize());
    settings.setValue("textColor", obsTextColor);
    settings.setValue("textBold", obsTextFontToSave.bold());
    settings.setValue("textItalic", obsTextFontToSave.italic());
    settings.setValue("textUnderline", obsTextFontToSave.underline());
    settings.setValue("textUppercase", obsTextUppercaseButton->isChecked());
    settings.setValue("textShadow", obsTextShadowCheck->isChecked());
    settings.setValue("textOutline", obsTextOutlineCheck->isChecked());
    settings.setValue("textOutlineWidth", obsTextOutlineWidthSpinBox->value());
    settings.setValue("textOutlineColor", obsTextOutlineColor);
    settings.setValue("textHighlight", obsTextHighlightCheck->isChecked());
    settings.setValue("textHighlightColor", obsTextHighlightColor);
    settings.setValue("textHighlightOpacity", obsTextHighlightOpacity);
    settings.setValue("textHighlightCornerRadius", obsTextHighlightCornerRadius);

    QFont obsRefFontToSave = fontFromControls(obsRefFontCombo,
                                              obsRefFontSizeSpinBox,
                                              obsRefBoldButton,
                                              obsRefItalicButton,
                                              obsRefUnderlineButton);
    settings.setValue("refFont", obsRefFontToSave.family());
    settings.setValue("refFontSize", obsRefFontToSave.pointSize());
    settings.setValue("refColor", obsRefColor);
    settings.setValue("refBold", obsRefFontToSave.bold());
    settings.setValue("refItalic", obsRefFontToSave.italic());
    settings.setValue("refUnderline", obsRefFontToSave.underline());
    settings.setValue("refUppercase", obsRefUppercaseButton->isChecked());
    settings.setValue("refShadow", obsRefShadowCheck->isChecked());
    settings.setValue("refOutline", obsRefOutlineCheck->isChecked());
    settings.setValue("refOutlineWidth", obsRefOutlineWidthSpinBox->value());
    settings.setValue("refOutlineColor", obsRefOutlineColor);
    settings.setValue("refHighlight", obsRefHighlightCheck->isChecked());
    settings.setValue("refHighlightColor", obsRefHighlightColor);
    settings.setValue("refHighlightOpacity", obsRefHighlightOpacity);
    settings.setValue("refHighlightCornerRadius", obsRefHighlightCornerRadius);
    settings.setValue("refDisplayVersion", obsRefDisplayVersionCheck->isChecked());
    settings.endGroup();
}

void SettingsDialog::onAccepted()
{
    saveSettings();
    accept();
}

void SettingsDialog::onRejected()
{
    reject();
}

void SettingsDialog::onProjectionTextColorClicked()
{
    QColor color = QColorDialog::getColor(projectionTextColor, this, "Choose Text Color");
    if (color.isValid()) {
        projectionTextColor = color;
        setButtonColorPreview(projectionTextColorButton, projectionTextColor);
    }
}

void SettingsDialog::onProjectionRefColorClicked()
{
    QColor color = QColorDialog::getColor(projectionRefColor, this, "Choose Reference Color");
    if (color.isValid()) {
        projectionRefColor = color;
        setButtonColorPreview(projectionRefColorButton, projectionRefColor);
    }
}

void SettingsDialog::onProjectionBgColorClicked()
{
    QColor color = QColorDialog::getColor(projectionBgColor, this, "Choose Background Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        projectionBgColor = color;
        projectionBgOpacity = qRound(projectionBgColor.alpha() * 100.0 / 255.0);
        {
            QSignalBlocker blocker(projectionBgOpacitySlider);
            projectionBgOpacitySlider->setValue(projectionBgOpacity);
        }
        {
            QSignalBlocker blocker(projectionBgOpacityValueLabel);
            projectionBgOpacityValueLabel->setValue(projectionBgOpacity);
        }
        setButtonColorPreview(projectionBgColorButton, projectionBgColor);
    }
}

void SettingsDialog::onProjectionRefHighlightToggled(bool checked)
{
    projectionRefHighlightColorButton->setEnabled(checked);
    projectionRefHighlightOpacitySlider->setEnabled(checked);
    projectionRefHighlightOpacityValueLabel->setEnabled(checked);
}

void SettingsDialog::onProjectionRefHighlightColorClicked()
{
    QColor color = QColorDialog::getColor(projectionRefHighlightColor, this, "Choose Reference Highlight Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        projectionRefHighlightColor = color;
        projectionRefHighlightOpacity = qRound(projectionRefHighlightColor.alpha() * 100.0 / 255.0);
        {
            QSignalBlocker blocker(projectionRefHighlightOpacitySlider);
            projectionRefHighlightOpacitySlider->setValue(projectionRefHighlightOpacity);
        }
        {
            QSignalBlocker blocker(projectionRefHighlightOpacityValueLabel);
            projectionRefHighlightOpacityValueLabel->setValue(projectionRefHighlightOpacity);
        }
        setButtonColorPreview(projectionRefHighlightColorButton, projectionRefHighlightColor);
    }
}

void SettingsDialog::onProjectionBgOpacityChanged(int value)
{
    projectionBgOpacity = qBound(0, value, 100);
    {
        QSignalBlocker blocker(projectionBgOpacityValueLabel);
        projectionBgOpacityValueLabel->setValue(projectionBgOpacity);
    }
    projectionBgColor.setAlpha(qRound(projectionBgOpacity * 255.0 / 100.0));
    setButtonColorPreview(projectionBgColorButton, projectionBgColor);
}

void SettingsDialog::onProjectionBgCornerRadiusChanged(int value)
{
    projectionBgCornerRadius = qBound(0, value, 200);
    {
        QSignalBlocker blocker(projectionBgCornerRadiusValueLabel);
        projectionBgCornerRadiusValueLabel->setValue(projectionBgCornerRadius);
    }
}

void SettingsDialog::onProjectionRefHighlightOpacityChanged(int value)
{
    projectionRefHighlightOpacity = qBound(0, value, 100);
    {
        QSignalBlocker blocker(projectionRefHighlightOpacityValueLabel);
        projectionRefHighlightOpacityValueLabel->setValue(projectionRefHighlightOpacity);
    }
    projectionRefHighlightColor.setAlpha(qRound(projectionRefHighlightOpacity * 255.0 / 100.0));
    setButtonColorPreview(projectionRefHighlightColorButton, projectionRefHighlightColor);
}

void SettingsDialog::onBibleTextSettingsClicked()
{
    showGroupInDialog(bibleTextGroup, this, "Bible Text Settings");
}

void SettingsDialog::onProjectionReferenceSettingsClicked()
{
    showGroupInDialog(projectionReferenceGroup, this, "Bible Reference Settings");
}

void SettingsDialog::onSongLyricsSettingsClicked()
{
    showGroupInDialog(songLyricsGroup, this, "Song Lyrics Settings");
}

void SettingsDialog::onProjectionRefHighlightCornerRadiusChanged(int value)
{
    projectionRefHighlightCornerRadius = qBound(0, value, 200);
    {
        QSignalBlocker blocker(projectionRefHighlightCornerRadiusValueLabel);
        projectionRefHighlightCornerRadiusValueLabel->setValue(projectionRefHighlightCornerRadius);
    }
}

void SettingsDialog::onSongTextColorClicked()
{
    QColor color = QColorDialog::getColor(songTextColor, this, "Choose Song Text Color");
    if (color.isValid()) {
        songTextColor = color;
        setButtonColorPreview(songTextColorButton, songTextColor);
    }
}

void SettingsDialog::onSongBgColorClicked()
{
    QColor color = QColorDialog::getColor(songBgColor, this, "Choose Song Background Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        songBgColor = color;
        songBgOpacity = qRound(songBgColor.alpha() * 100.0 / 255.0);
        {
            QSignalBlocker blocker(songBgOpacitySlider);
            songBgOpacitySlider->setValue(songBgOpacity);
        }
        {
            QSignalBlocker blocker(songBgOpacityValueLabel);
            songBgOpacityValueLabel->setValue(songBgOpacity);
        }
        setButtonColorPreview(songBgColorButton, songBgColor);
    }
}

void SettingsDialog::onSongBgOpacityChanged(int value)
{
    songBgOpacity = qBound(0, value, 100);
    {
        QSignalBlocker blocker(songBgOpacityValueLabel);
        songBgOpacityValueLabel->setValue(songBgOpacity);
    }
    songBgColor.setAlpha(qRound(songBgOpacity * 255.0 / 100.0));
    setButtonColorPreview(songBgColorButton, songBgColor);
}

void SettingsDialog::onSongBgCornerRadiusChanged(int value)
{
    songBgCornerRadius = qBound(0, value, 200);
    {
        QSignalBlocker blocker(songBgCornerRadiusValueLabel);
        songBgCornerRadiusValueLabel->setValue(songBgCornerRadius);
    }
}

void SettingsDialog::onSongLineSpacingChanged(int value)
{
    songLineSpacingPercent = qBound(100, value, 200);
    {
        QSignalBlocker blocker(songLineSpacingValueLabel);
        songLineSpacingValueLabel->setValue(songLineSpacingPercent);
    }
    updateSongBorderControls();
}

void SettingsDialog::onSongBorderToggled(bool checked)
{
    songBorderEnabled = checked;
    updateSongBorderControls();
}

void SettingsDialog::onSongBorderThicknessChanged(int value)
{
    songBorderThickness = qBound(1, value, 20);
    {
        QSignalBlocker blocker(songBorderThicknessValueLabel);
        songBorderThicknessValueLabel->setValue(songBorderThickness);
    }
}

void SettingsDialog::onSongBorderColorClicked()
{
    QColor color = QColorDialog::getColor(songBorderColor, this, "Choose Song Border Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        songBorderColor = color;
        setButtonColorPreview(songBorderColorButton, songBorderColor);
    }
}

void SettingsDialog::onDefaultBibleVersionChanged(int index)
{
    Q_UNUSED(index);
    if (!defaultBibleVersionCombo) {
        return;
    }
    defaultBibleTranslationPath = defaultBibleVersionCombo->currentData().toString();
}

void SettingsDialog::onSongBorderHorizontalPaddingChanged(int value)
{
    songBorderPaddingHorizontal = qBound(0, value, 400);
    {
        QSignalBlocker blocker(songBorderPaddingHorizontalValueLabel);
        songBorderPaddingHorizontalValueLabel->setValue(songBorderPaddingHorizontal);
    }
}

void SettingsDialog::onSongBorderVerticalPaddingChanged(int value)
{
    songBorderPaddingVertical = qBound(0, value, 400);
    {
        QSignalBlocker blocker(songBorderPaddingVerticalValueLabel);
        songBorderPaddingVerticalValueLabel->setValue(songBorderPaddingVertical);
    }
}

void SettingsDialog::onSeparateBackgroundsToggled(bool checked)
{
    useSeparateBackgrounds(checked);
}

void SettingsDialog::onVerseBackgroundColorClicked()
{
    QColor color = QColorDialog::getColor(verseBackgroundColor, this, "Choose Verse Background Color");
    if (color.isValid()) {
        verseBackgroundColor = color;
        if (!separateBackgroundsCheck->isChecked()) {
            songBackgroundColor = color;
        }
    }
}

void SettingsDialog::onSongBackgroundColorClicked()
{
    QColor color = QColorDialog::getColor(songBackgroundColor, this, "Choose Song Background Color");
    if (color.isValid()) {
        songBackgroundColor = color;
    }
}

void SettingsDialog::onBrowseVerseBackgroundImage()
{
#ifdef Q_OS_MACOS
    QFileDialog dialog(this, "Select Verse Background Image");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory("data/backgrounds");
    dialog.setNameFilter("Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() == QDialog::Accepted) {
        const QString fileName = dialog.selectedFiles().value(0);
        if (!fileName.isEmpty()) {
            verseBackgroundImageEdit->setText(fileName);
            if (!separateBackgroundsCheck->isChecked()) {
                songBackgroundImageEdit->setText(fileName);
            }
        }
    }
#else
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Verse Background Image",
        "data/backgrounds",
        "Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        verseBackgroundImageEdit->setText(fileName);
        if (!separateBackgroundsCheck->isChecked()) {
            songBackgroundImageEdit->setText(fileName);
        }
    }
#endif
}

void SettingsDialog::onBrowseVerseBackgroundVideo()
{
#ifdef Q_OS_MACOS
    QFileDialog dialog(this, "Select Verse Background Video");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory("data/backgrounds");
    dialog.setNameFilter("Videos (*.mp4 *.webm *.avi *.mov);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() == QDialog::Accepted) {
        const QString fileName = dialog.selectedFiles().value(0);
        if (!fileName.isEmpty()) {
            verseBackgroundVideoEdit->setText(fileName);
            if (!separateBackgroundsCheck->isChecked()) {
                songBackgroundVideoEdit->setText(fileName);
            }
        }
    }
#else
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Verse Background Video",
        "data/backgrounds",
        "Videos (*.mp4 *.webm *.avi *.mov);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        verseBackgroundVideoEdit->setText(fileName);
        if (!separateBackgroundsCheck->isChecked()) {
            songBackgroundVideoEdit->setText(fileName);
        }
    }
#endif
}

void SettingsDialog::onBrowseSongBackgroundImage()
{
#ifdef Q_OS_MACOS
    QFileDialog dialog(this, "Select Song Background Image");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory("data/backgrounds");
    dialog.setNameFilter("Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() == QDialog::Accepted) {
        const QString fileName = dialog.selectedFiles().value(0);
        if (!fileName.isEmpty()) {
            songBackgroundImageEdit->setText(fileName);
        }
    }
#else
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Song Background Image",
        "data/backgrounds",
        "Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        songBackgroundImageEdit->setText(fileName);
    }
#endif
}

void SettingsDialog::onBrowseSongBackgroundVideo()
{
#ifdef Q_OS_MACOS
    QFileDialog dialog(this, "Select Song Background Video");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory("data/backgrounds");
    dialog.setNameFilter("Videos (*.mp4 *.webm *.avi *.mov);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() == QDialog::Accepted) {
        const QString fileName = dialog.selectedFiles().value(0);
        if (!fileName.isEmpty()) {
            songBackgroundVideoEdit->setText(fileName);
        }
    }
#else
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Song Background Video",
        "data/backgrounds",
        "Videos (*.mp4 *.webm *.avi *.mov);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        songBackgroundVideoEdit->setText(fileName);
    }
#endif
}

void SettingsDialog::onNotesBackgroundColorClicked()
{
    QColor color = QColorDialog::getColor(notesBackgroundColor, this, "Choose Notes Background Color");
    if (color.isValid()) {
        notesBackgroundColor = color;
    }
}

void SettingsDialog::onBrowseNotesBackgroundImage()
{
#ifdef Q_OS_MACOS
    QFileDialog dialog(this, "Select Notes Background Image");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory("data/backgrounds");
    dialog.setNameFilter("Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() == QDialog::Accepted) {
        const QString fileName = dialog.selectedFiles().value(0);
        if (!fileName.isEmpty()) {
            notesBackgroundImageEdit->setText(fileName);
        }
    }
#else
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Notes Background Image",
        "data/backgrounds",
        "Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        notesBackgroundImageEdit->setText(fileName);
    }
#endif
}

void SettingsDialog::onBrowseNotesBackgroundVideo()
{
#ifdef Q_OS_MACOS
    QFileDialog dialog(this, "Select Notes Background Video");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory("data/backgrounds");
    dialog.setNameFilter("Videos (*.mp4 *.webm *.avi *.mov);;All Files (*)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() == QDialog::Accepted) {
        const QString fileName = dialog.selectedFiles().value(0);
        if (!fileName.isEmpty()) {
            notesBackgroundVideoEdit->setText(fileName);
        }
    }
#else
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Notes Background Video",
        "data/backgrounds",
        "Videos (*.mp4 *.webm *.avi *.mov);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        notesBackgroundVideoEdit->setText(fileName);
    }
#endif
}

void SettingsDialog::onOBSTextColorClicked()
{
    QColor color = QColorDialog::getColor(obsTextColor, this, "Choose OBS Text Color");
    if (color.isValid()) {
        obsTextColor = color;
        setButtonColorPreview(obsTextColorButton, obsTextColor);
    }
}

void SettingsDialog::onOBSRefColorClicked()
{
    QColor color = QColorDialog::getColor(obsRefColor, this, "Choose OBS Reference Color");
    if (color.isValid()) {
        obsRefColor = color;
        setButtonColorPreview(obsRefColorButton, obsRefColor);
    }
}

void SettingsDialog::onOBSTextHighlightColorClicked()
{
    QColor color = QColorDialog::getColor(obsTextHighlightColor, this, "Choose OBS Text Highlight Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        obsTextHighlightColor = color;
        setButtonColorPreview(obsTextHighlightColorButton, obsTextHighlightColor);
    }
}

void SettingsDialog::onOBSTextHighlightOpacityChanged(int value)
{
    obsTextHighlightOpacity = qBound(0, value, 100);
    {
        QSignalBlocker blocker(obsTextHighlightOpacityLabel);
        obsTextHighlightOpacityLabel->setValue(obsTextHighlightOpacity);
    }
    obsTextHighlightColor.setAlpha(qRound(obsTextHighlightOpacity * 255.0 / 100.0));
}

void SettingsDialog::onOBSTextHighlightCornerRadiusChanged(int value)
{
    obsTextHighlightCornerRadius = qBound(0, value, 200);
    {
        QSignalBlocker blocker(obsTextHighlightCornerRadiusLabel);
        obsTextHighlightCornerRadiusLabel->setValue(obsTextHighlightCornerRadius);
    }
}

void SettingsDialog::onOBSTextHighlightToggled(bool checked)
{
    obsTextHighlightOpacitySlider->setEnabled(checked);
    obsTextHighlightOpacityLabel->setEnabled(checked);
    obsTextHighlightCornerRadiusSlider->setEnabled(checked);
    obsTextHighlightCornerRadiusLabel->setEnabled(checked);
}

void SettingsDialog::onOBSRefHighlightColorClicked()
{
    QColor color = QColorDialog::getColor(obsRefHighlightColor, this, "Choose OBS Reference Highlight Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        obsRefHighlightColor = color;
        setButtonColorPreview(obsRefHighlightColorButton, obsRefHighlightColor);
    }
}

void SettingsDialog::onOBSRefHighlightOpacityChanged(int value)
{
    obsRefHighlightOpacity = qBound(0, value, 100);
    {
        QSignalBlocker blocker(obsRefHighlightOpacityLabel);
        obsRefHighlightOpacityLabel->setValue(obsRefHighlightOpacity);
    }
    obsRefHighlightColor.setAlpha(qRound(obsRefHighlightOpacity * 255.0 / 100.0));
}

void SettingsDialog::onOBSRefHighlightCornerRadiusChanged(int value)
{
    obsRefHighlightCornerRadius = qBound(0, value, 200);
    {
        QSignalBlocker blocker(obsRefHighlightCornerRadiusLabel);
        obsRefHighlightCornerRadiusLabel->setValue(obsRefHighlightCornerRadius);
    }
}

void SettingsDialog::onOBSRefHighlightToggled(bool checked)
{
    obsRefHighlightOpacitySlider->setEnabled(checked);
    obsRefHighlightOpacityLabel->setEnabled(checked);
    obsRefHighlightCornerRadiusSlider->setEnabled(checked);
    obsRefHighlightCornerRadiusLabel->setEnabled(checked);
}

void SettingsDialog::onOBSTextOutlineColorClicked()
{
    QColor color = QColorDialog::getColor(obsTextOutlineColor, this, "Choose OBS Text Outline Color");
    if (color.isValid()) {
        obsTextOutlineColor = color;
        setButtonColorPreview(obsTextOutlineColorButton, obsTextOutlineColor);
    }
}

void SettingsDialog::onOBSRefOutlineColorClicked()
{
    QColor color = QColorDialog::getColor(obsRefOutlineColor, this, "Choose OBS Reference Outline Color");
    if (color.isValid()) {
        obsRefOutlineColor = color;
        setButtonColorPreview(obsRefOutlineColorButton, obsRefOutlineColor);
    }
}

void SettingsDialog::showTextFormatting()
{
    QDialog *textDialog = new QDialog(this);
    textDialog->setWindowTitle("Text Formatting");
    textDialog->setMinimumWidth(500);
    
    QVBoxLayout *dialogLayout = new QVBoxLayout(textDialog);
    
    // Create scroll area
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    
    // Find the hidden text formatting group
    QGroupBox *textGroup = nullptr;
    QList<QGroupBox*> groups = findChildren<QGroupBox*>();
    for (QGroupBox *group : groups) {
        if (group->title() == "Text Formatting" && !group->isVisible()) {
            textGroup = group;
            break;
        }
    }
    
    if (textGroup) {
        textGroup->setVisible(true);
        textGroup->setParent(scrollWidget);
        scrollLayout->addWidget(textGroup);
    }
    
    scrollArea->setWidget(scrollWidget);
    dialogLayout->addWidget(scrollArea);
    
    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, textDialog, &QDialog::accept);
    dialogLayout->addWidget(buttonBox);
    
    textDialog->exec();
    
    // Return widget to hidden state
    if (textGroup) {
        textGroup->setParent(this);
        textGroup->setVisible(false);
    }
    
    delete textDialog;
}

void SettingsDialog::showReferenceFormatting()
{
    QDialog *refDialog = new QDialog(this);
    refDialog->setWindowTitle("Bible Reference Formatting");
    refDialog->setMinimumWidth(500);
    
    QVBoxLayout *dialogLayout = new QVBoxLayout(refDialog);
    
    // Create scroll area
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    
    // Find the hidden reference formatting group
    QGroupBox *refGroup = nullptr;
    QList<QGroupBox*> groups = findChildren<QGroupBox*>();
    for (QGroupBox *group : groups) {
        if (group->title() == "Reference Formatting" && !group->isVisible()) {
            refGroup = group;
            break;
        }
    }
    
    if (refGroup) {
        refGroup->setVisible(true);
        refGroup->setParent(scrollWidget);
        scrollLayout->addWidget(refGroup);
    }
    
    scrollArea->setWidget(scrollWidget);
    dialogLayout->addWidget(scrollArea);
    
    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, refDialog, &QDialog::accept);
    dialogLayout->addWidget(buttonBox);
    
    refDialog->exec();
    
    // Return widget to hidden state
    if (refGroup) {
        refGroup->setParent(this);
        refGroup->setVisible(false);
    }
    
    delete refDialog;
}
