#ifndef POWERPOINTPANEL_H
#define POWERPOINTPANEL_H

#include <QWidget>
#include <QStringList>

class QPushButton;
class QLabel;
class QListWidget;
class QSlider;
class QCheckBox;
class QProgressBar;

// Forward declare QAxObject when ActiveQt is available
#ifdef SIMPLEPRESENTER_HAVE_ACTIVEQT
class QAxObject;
#endif

// PowerPoint integration requires ActiveQt (AxContainer).
// If SIMPLEPRESENTER_HAVE_ACTIVEQT is not defined, this panel will act as
// a stub and show a "not available" message.

class PowerPointPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PowerPointPanel(QWidget *parent = nullptr);
    ~PowerPointPanel();

signals:
    void presentationOpened(const QString &path);
    void slideImageAvailable(const QString &imagePath, bool useFade);

private slots:
    void onOpenPresentation();
    void onStartSlideShow();
    void onNextSlide();
    void onPreviousSlide();
    void onEndSlideShow();

private:
    void setupUI();
    void ensurePowerPoint();
    bool hasActiveSlideShow() const;
    QString exportCurrentSlideAsImage();
    void rebuildThumbnails();
    void setLoading(bool loading);

    // LibreOffice-based slide conversion (used primarily on macOS when
    // ActiveQt/PowerPoint automation is not available).
    QString findLibreOfficeExecutable() const;
    bool convertPresentationWithLibreOffice(const QString &filePath,
                                            QString &outputDir,
                                            QString &errorMessage);

    QPushButton *openButton;
    QPushButton *startButton;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QPushButton *endButton;
    QLabel *statusLabel;
    QListWidget *thumbnailList;
    QSlider *thumbnailZoomSlider;
    QCheckBox *fadeCheckBox;
    QProgressBar *loadingProgress;

    QString currentPresentationPath;
    QStringList slideImagePaths;
    QString libreOfficeExecutable;

#ifdef SIMPLEPRESENTER_HAVE_ACTIVEQT
    QAxObject *pptApp;
    QAxObject *pptPresentation;
#else
    void *pptApp;
    void *pptPresentation;
#endif

    int currentSlideIndex;
    int totalSlideCount;
};

#endif // POWERPOINTPANEL_H
