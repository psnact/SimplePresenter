#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include <QObject>

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE

#include <QSet>
#include <QUrl>
#include <QWebEngineUrlRequestInterceptor>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
class QWebEngineProfile;
class QWebEngineUrlRequestInfo;

class SimpleFilterEngine : public QObject
{
    Q_OBJECT
public:
    explicit SimpleFilterEngine(QObject *parent = nullptr);

    void setHostRules(const QSet<QString> &blockedHosts,
                      const QSet<QString> &exceptionHosts);

    bool shouldBlock(const QUrl &requestUrl, const QUrl &firstPartyUrl) const;

private:
    QSet<QString> blocked;
    QSet<QString> exceptions;
};

class AdblockUrlRequestInterceptor : public QWebEngineUrlRequestInterceptor
{
public:
    explicit AdblockUrlRequestInterceptor(SimpleFilterEngine *engine, QObject *parent = nullptr);

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

private:
    SimpleFilterEngine *filterEngine;
};

class AdblockManager : public QObject
{
    Q_OBJECT
public:
    explicit AdblockManager(QObject *parent = nullptr);

private slots:
    void checkForUpdates();
    void onDownloadFinished(QNetworkReply *reply);

private:
    void loadCachedRules();
    void parseAndInstall(const QByteArray &data);
    void scheduleNextUpdate(int minutes);

    QString cacheFilePath() const;

    QNetworkAccessManager *network;
    QTimer *updateTimer;
    SimpleFilterEngine engine;
    AdblockUrlRequestInterceptor *interceptor;
};

#endif // SIMPLEPRESENTER_HAVE_WEBENGINE

#endif // ADBLOCKMANAGER_H
