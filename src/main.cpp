#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QIcon>
#include <QStandardPaths>
#include <QSplashScreen>
#include <QPixmap>
#include "MainWindow.h"
#include "BibleManager.h"
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
#include "AdblockManager.h"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application metadata
    app.setApplicationName("SimplePresenter");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SimplePresenter");
    
    // Use Fusion style for modern look
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Create data directories if they don't exist
    QDir().mkpath(BibleManager::bibleDirectory());
    QDir().mkpath("data/songs");
    QDir().mkpath("data/services");
    QDir().mkpath("data/backgrounds");

    // Set application icon from resources
    QIcon appIcon(QStringLiteral(":/Logo.ico"));
    app.setWindowIcon(appIcon);

    QPixmap splashPixmap(QStringLiteral(":/SPlogo.png"));
    if (!splashPixmap.isNull()) {
        // Make the splash logo a bit smaller than the original while
        // preserving aspect ratio.
        splashPixmap = splashPixmap.scaled(splashPixmap.size() / 2,
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
    }
    QSplashScreen splash(splashPixmap);
    splash.show();
    app.processEvents();

    // Optional: start adblock manager for embedded WebEngine views
#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE
    AdblockManager adblockManager;
#endif

    // Create and show main window
    MainWindow mainWindow;
    mainWindow.setWindowIcon(appIcon);
    mainWindow.show();

    splash.finish(&mainWindow);
    
    int result = app.exec();

    // Cleanup generated PowerPoint slide images under the application data directory
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!baseDir.isEmpty()) {
        QDir slidesDir(baseDir);
        if (slidesDir.cd("slides")) {
            slidesDir.removeRecursively();
        }
    }

    // Cleanup temporary copied PowerPoint presentations from the system temp folder
    QString tempRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempRoot.isEmpty()) {
        tempRoot = QDir::tempPath();
    }
    if (!tempRoot.isEmpty()) {
        QDir tempDir(tempRoot);
        if (tempDir.cd("SimplePresenterPPT")) {
            tempDir.removeRecursively();
        }
    }

    return result;
}
