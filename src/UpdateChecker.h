#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);
    void checkForUpdates(const QString &url);

signals:
    void updateAvailable(const QString &version, const QString &downloadUrl, const QString &releaseNotes);
    void noUpdateAvailable();
    void checkFailed(const QString &error);

private slots:
    void onResult(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
};

#endif // UPDATECHECKER_H
