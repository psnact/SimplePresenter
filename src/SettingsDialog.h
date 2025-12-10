#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontComboBox>
#include <QToolButton>
#include <QSlider>
#include <QLabel>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void onAccepted();
    void onRejected();
    void onProjectionTextColorClicked();
    void onProjectionRefColorClicked();
    void onProjectionBgColorClicked();
    void onProjectionRefHighlightToggled(bool checked);
    void onBibleTextSettingsClicked();
    void onProjectionReferenceSettingsClicked();
    void onSongLyricsSettingsClicked();
    void onProjectionRefHighlightColorClicked();
    void onProjectionBgOpacityChanged(int value);
    void onProjectionBgCornerRadiusChanged(int value);
    void onProjectionRefHighlightOpacityChanged(int value);
    void onProjectionRefHighlightCornerRadiusChanged(int value);
    void onSongTextColorClicked();
    void onSongBgColorClicked();
    void onSongBgOpacityChanged(int value);
    void onSongBgCornerRadiusChanged(int value);
    void onSongLineSpacingChanged(int value);
    void onSongBorderToggled(bool checked);
    void onSongBorderThicknessChanged(int value);
    void onSongBorderColorClicked();
    void onSongBorderHorizontalPaddingChanged(int value);
    void onSongBorderVerticalPaddingChanged(int value);
    void onDefaultBibleVersionChanged(int index);
    void onSeparateBackgroundsToggled(bool checked);
    void onVerseBackgroundColorClicked();
    void onSongBackgroundColorClicked();
    void onBrowseVerseBackgroundImage();
    void onBrowseVerseBackgroundVideo();
    void onBrowseSongBackgroundImage();
    void onBrowseSongBackgroundVideo();
    void onNotesBackgroundColorClicked();
    void onBrowseNotesBackgroundImage();
    void onBrowseNotesBackgroundVideo();
    void onOBSTextColorClicked();
    void onOBSRefColorClicked();
    void onOBSTextHighlightColorClicked();
    void onOBSTextHighlightOpacityChanged(int value);
    void onOBSTextHighlightCornerRadiusChanged(int value);
    void onOBSTextOutlineColorClicked();
    void onOBSRefOutlineColorClicked();
    void onOBSRefHighlightColorClicked();
    void onOBSRefHighlightOpacityChanged(int value);
    void onOBSRefHighlightCornerRadiusChanged(int value);
    void onOBSTextHighlightToggled(bool checked);
    void onOBSRefHighlightToggled(bool checked);
    void showTextFormatting();
    void showReferenceFormatting();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void useSeparateBackgrounds(bool separate);
    void syncSongBackgroundToVerse();
    void updateSongBorderControls();
    void populateDefaultBibleVersions();
    
    QTabWidget *tabWidget;
    
    // Projection canvas settings
    QComboBox *displayCombo;
    QFontComboBox *projectionFontCombo;
    QSpinBox *projectionFontSizeSpinBox;
    QToolButton *projectionFontBoldButton;
    QToolButton *projectionFontItalicButton;
    QToolButton *projectionFontUnderlineButton;
    QToolButton *projectionFontUppercaseButton;
    QFontComboBox *projectionRefFontCombo;
    QSpinBox *projectionRefFontSizeSpinBox;
    QToolButton *projectionRefFontBoldButton;
    QToolButton *projectionRefFontItalicButton;
    QToolButton *projectionRefFontUnderlineButton;
    QToolButton *projectionRefFontUppercaseButton;
    QGroupBox *bibleTextGroup;
    QGroupBox *projectionReferenceGroup;
    QGroupBox *songLyricsGroup;
    QComboBox *defaultBibleVersionCombo;
    QPushButton *projectionTextColorButton;
    QPushButton *projectionRefColorButton;
    QCheckBox *projectionRefHighlightCheck;
    QPushButton *projectionRefHighlightColorButton;
    QSlider *projectionRefHighlightOpacitySlider;
    QSpinBox *projectionRefHighlightOpacityValueLabel;
    QSlider *projectionRefHighlightCornerRadiusSlider;
    QSpinBox *projectionRefHighlightCornerRadiusValueLabel;
    QPushButton *projectionBgColorButton;
    QSlider *projectionBgOpacitySlider;
    QSpinBox *projectionBgOpacityValueLabel;
    QSlider *projectionBgCornerRadiusSlider;
    QSpinBox *projectionBgCornerRadiusValueLabel;
    QComboBox *projectionAlignmentCombo;
    QComboBox *projectionRefAlignmentCombo;
    QComboBox *projectionRefPositionCombo;
    QComboBox *projectionPositionCombo;
    QCheckBox *projectionSeparateReferenceCheck;
    QCheckBox *projectionDisplayVersionCheck;
    QColor projectionTextColor;
    QColor projectionRefColor;
    QColor projectionBgColor;
    QColor projectionRefHighlightColor;
    int projectionBgOpacity;
    int projectionBgCornerRadius;
    int projectionRefHighlightOpacity;
    int projectionRefHighlightCornerRadius;
    QString defaultBibleTranslationPath;

    // Song lyrics settings
    QFontComboBox *songFontCombo;
    QSpinBox *songFontSizeSpinBox;
    QToolButton *songFontBoldButton;
    QToolButton *songFontItalicButton;
    QToolButton *songFontUnderlineButton;
    QToolButton *songFontUppercaseButton;
    QPushButton *songTextColorButton;
    QPushButton *songBgColorButton;
    QSlider *songBgOpacitySlider;
    QSpinBox *songBgOpacityValueLabel;
    QSlider *songBgCornerRadiusSlider;
    QSpinBox *songBgCornerRadiusValueLabel;
    QSlider *songLineSpacingSlider;
    QSpinBox *songLineSpacingValueLabel;
    QCheckBox *songBorderEnabledCheck;
    QPushButton *songBorderColorButton;
    QSlider *songBorderThicknessSlider;
    QSpinBox *songBorderThicknessValueLabel;
    QSlider *songBorderPaddingHorizontalSlider;
    QSpinBox *songBorderPaddingHorizontalValueLabel;
    QSlider *songBorderPaddingVerticalSlider;
    QSpinBox *songBorderPaddingVerticalValueLabel;
    QComboBox *songAlignmentCombo;
    QComboBox *songPositionCombo;
    QColor songTextColor;
    QColor songBgColor;
    QColor songBorderColor;
    int songBgOpacity;
    int songBgCornerRadius;
    int songLineSpacingPercent;
    bool songBorderEnabled;
    int songBorderThickness;
    int songBorderPaddingHorizontal;
    int songBorderPaddingVertical;
    
    // Background settings
    QCheckBox *separateBackgroundsCheck;
    QGroupBox *songBackgroundGroup;

    // Verse background controls
    QComboBox *verseBackgroundTypeCombo;
    QPushButton *verseBackgroundColorButton;
    QLineEdit *verseBackgroundImageEdit;
    QLineEdit *verseBackgroundVideoEdit;
    QPushButton *verseBrowseImageButton;
    QPushButton *verseBrowseVideoButton;
    QCheckBox *verseVideoLoopCheck;
    QColor verseBackgroundColor;

    // Song background controls
    QComboBox *songBackgroundTypeCombo;
    QPushButton *songBackgroundColorButton;
    QLineEdit *songBackgroundImageEdit;
    QLineEdit *songBackgroundVideoEdit;
    QPushButton *songBrowseImageButton;
    QPushButton *songBrowseVideoButton;
    QCheckBox *songVideoLoopCheck;
    QColor songBackgroundColor;

    // Notes background controls
    QGroupBox *notesBackgroundGroup;
    QComboBox *notesBackgroundTypeCombo;
    QPushButton *notesBackgroundColorButton;
    QLineEdit *notesBackgroundImageEdit;
    QLineEdit *notesBackgroundVideoEdit;
    QPushButton *notesBrowseImageButton;
    QPushButton *notesBrowseVideoButton;
    QCheckBox *notesVideoLoopCheck;
    QColor notesBackgroundColor;
    
    // OBS Overlay settings
    QFontComboBox *obsTextFontCombo;
    QSpinBox *obsTextFontSizeSpinBox;
    QPushButton *obsTextColorButton;
    QToolButton *obsTextBoldButton;
    QToolButton *obsTextItalicButton;
    QToolButton *obsTextUnderlineButton;
    QToolButton *obsTextUppercaseButton;
    QCheckBox *obsTextShadowCheck;
    QCheckBox *obsTextOutlineCheck;
    QSpinBox *obsTextOutlineWidthSpinBox;
    QPushButton *obsTextOutlineColorButton;
    QCheckBox *obsTextHighlightCheck;
    QPushButton *obsTextHighlightColorButton;
    QSlider *obsTextHighlightOpacitySlider;
    QSpinBox *obsTextHighlightOpacityLabel;
    QSlider *obsTextHighlightCornerRadiusSlider;
    QSpinBox *obsTextHighlightCornerRadiusLabel;
    
    QFontComboBox *obsRefFontCombo;
    QSpinBox *obsRefFontSizeSpinBox;
    QPushButton *obsRefColorButton;
    QToolButton *obsRefBoldButton;
    QToolButton *obsRefItalicButton;
    QToolButton *obsRefUnderlineButton;
    QToolButton *obsRefUppercaseButton;
    QCheckBox *obsRefShadowCheck;
    QCheckBox *obsRefOutlineCheck;
    QSpinBox *obsRefOutlineWidthSpinBox;
    QPushButton *obsRefOutlineColorButton;
    QCheckBox *obsRefHighlightCheck;
    QPushButton *obsRefHighlightColorButton;
    QSlider *obsRefHighlightOpacitySlider;
    QSpinBox *obsRefHighlightOpacityLabel;
    QSlider *obsRefHighlightCornerRadiusSlider;
    QSpinBox *obsRefHighlightCornerRadiusLabel;
    QCheckBox *obsRefDisplayVersionCheck;
    
    QColor obsTextColor;
    QColor obsRefColor;
    QColor obsTextHighlightColor;
    QColor obsRefHighlightColor;
    int obsTextHighlightOpacity;
    int obsTextHighlightCornerRadius;
    int obsRefHighlightOpacity;
    int obsRefHighlightCornerRadius;
    QColor obsTextOutlineColor;
    QColor obsRefOutlineColor;
    
    // OBS Canvas Resolution
    QComboBox *obsResolutionCombo;
};

#endif // SETTINGSDIALOG_H
