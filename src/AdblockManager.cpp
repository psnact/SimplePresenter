#include "AdblockManager.h"

#ifdef SIMPLEPRESENTER_HAVE_WEBENGINE

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineUrlRequestInterceptor>
#include <QDebug>

static const char *kDefaultFilterListUrl =
    "https://raw.githubusercontent.com/uBlockOrigin/uAssets/master/filters.txt";
static const int kUpdateIntervalMinutes = 60 * 12; // 12 hours

// ---------------- SimpleFilterEngine ----------------

SimpleFilterEngine::SimpleFilterEngine(QObject *parent)
    : QObject(parent)
{
}

void SimpleFilterEngine::setHostRules(const QSet<QString> &blockedHosts,
                                      const QSet<QString> &exceptionHosts)
{
    blocked = blockedHosts;
    exceptions = exceptionHosts;
}

bool SimpleFilterEngine::shouldBlock(const QUrl &requestUrl, const QUrl &firstPartyUrl) const
{
    const QString host = requestUrl.host().toLower();
    if (host.isEmpty()) {
        return false;
    }

    const QString firstPartyHost = firstPartyUrl.host().toLower();

    QString current = host;
    while (!current.isEmpty()) {
        if (exceptions.contains(current)) {
            return false;
        }
        if (blocked.contains(current)) {
            // Basic third-party check: do not treat same-site as third-party
            if (!firstPartyHost.isEmpty()) {
                if (current == firstPartyHost || host.endsWith("." + current)) {
                    return false;
                }
            }
            return true;
        }
        int dot = current.indexOf('.');
        if (dot < 0) {
            break;
        }
        current = current.mid(dot + 1);
    }

    return false;
}

// ---------------- AdblockUrlRequestInterceptor ----------------

AdblockUrlRequestInterceptor::AdblockUrlRequestInterceptor(SimpleFilterEngine *engine, QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
    , filterEngine(engine)
{
}

void AdblockUrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (!filterEngine) {
        return;
    }

    const QUrl requestUrl = info.requestUrl();
    const QUrl firstPartyUrl = info.firstPartyUrl();
    bool block = false;

    // 1) Generic host-based blocking from filter lists
    if (filterEngine->shouldBlock(requestUrl, firstPartyUrl)) {
        block = true;
    }

    // 2) Very small YouTube-specific handler: when the main page is YouTube,
    //    block some well-known ad/tracking endpoints. This is intentionally
    //    conservative and does NOT attempt to block googlevideo.com segments
    //    to avoid breaking normal playback.
    if (!block) {
        const QString firstHost = firstPartyUrl.host().toLower();
        const QString host = requestUrl.host().toLower();
        const QString path = requestUrl.path();

        if (firstHost.contains(QStringLiteral("youtube.com"))) {
            if (host.endsWith(QStringLiteral(".doubleclick.net")) ||
                host.contains(QStringLiteral("googleads.")) ||
                host.contains(QStringLiteral(".googlesyndication."))) {
                block = true;
            } else if (host.contains(QStringLiteral("youtube.com")) &&
                       (path.contains(QStringLiteral("/pagead/")) ||
                        path.contains(QStringLiteral("/ptracking")))) {
                block = true;
            }
        }
    }

    if (block) {
        info.block(true);
    }
}

// ---------------- AdblockManager ----------------

AdblockManager::AdblockManager(QObject *parent)
    : QObject(parent)
    , network(new QNetworkAccessManager(this))
    , updateTimer(new QTimer(this))
    , engine(this)
    , interceptor(new AdblockUrlRequestInterceptor(&engine, this))
{
    QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();
    if (profile) {
        profile->setUrlRequestInterceptor(interceptor);
    }

    connect(updateTimer, &QTimer::timeout, this, &AdblockManager::checkForUpdates);
    updateTimer->setSingleShot(false);

    connect(network, &QNetworkAccessManager::finished,
            this, &AdblockManager::onDownloadFinished);

    loadCachedRules();
    scheduleNextUpdate(1); // Check shortly after startup
}

QString AdblockManager::cacheFilePath() const
{
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(baseDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath("adblock-filters.txt");
}

void AdblockManager::loadCachedRules()
{
    QFile file(cacheFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    parseAndInstall(data);
}

void AdblockManager::checkForUpdates()
{
    QNetworkRequest req(QUrl(QString::fromLatin1(kDefaultFilterListUrl)));
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("SimplePresenter-Adblock/1.0"));
    network->get(req);
}

void AdblockManager::onDownloadFinished(QNetworkReply *reply)
{
    if (!reply) {
        return;
    }

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "AdblockManager: download error" << reply->errorString();
        scheduleNextUpdate(kUpdateIntervalMinutes);
        return;
    }

    const QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        scheduleNextUpdate(kUpdateIntervalMinutes);
        return;
    }

    QFile file(cacheFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        file.write(data);
        file.close();
    }

    parseAndInstall(data);
    scheduleNextUpdate(kUpdateIntervalMinutes);
}

void AdblockManager::scheduleNextUpdate(int minutes)
{
    if (minutes <= 0) {
        minutes = 5;
    }
    updateTimer->start(minutes * 60 * 1000);
}

void AdblockManager::parseAndInstall(const QByteArray &data)
{
    QSet<QString> blockedHosts;
    QSet<QString> exceptionHosts;

    const QList<QByteArray> lines = data.split('\n');
    for (QByteArray rawLine : lines) {
        if (rawLine.isEmpty()) {
            continue;
        }
        if (rawLine.endsWith('\r')) {
            rawLine.chop(1);
        }
        QString line = QString::fromUtf8(rawLine).trimmed();
        if (line.isEmpty()) {
            continue;
        }
        if (line.startsWith('!')) {
            continue; // comment
        }

        bool isException = false;
        if (line.startsWith("@@")) {
            isException = true;
            line = line.mid(2);
        }

        if (!line.startsWith("||")) {
            continue;
        }

        int dollarIndex = line.indexOf('$');
        if (dollarIndex >= 0) {
            line = line.left(dollarIndex);
        }

        int start = 2; // after ||
        int end = line.length();
        for (int i = start; i < line.length(); ++i) {
            const QChar ch = line.at(i);
            if (ch == '^' || ch == '/' || ch == '?' || ch == '#') {
                end = i;
                break;
            }
        }
        QString host = line.mid(start, end - start).toLower();
        if (host.isEmpty()) {
            continue;
        }

        if (host.startsWith('.')) {
            host = host.mid(1);
        }

        if (isException) {
            exceptionHosts.insert(host);
        } else {
            blockedHosts.insert(host);
        }
    }

    engine.setHostRules(blockedHosts, exceptionHosts);
}

#endif // SIMPLEPRESENTER_HAVE_WEBENGINE
