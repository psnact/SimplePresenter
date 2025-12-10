#include "UpdateChecker.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>
#include <QApplication>

// If SIMPLEPRESENTER_VERSION is not defined, default to 1.0.0
#ifndef SIMPLEPRESENTER_VERSION
#define SIMPLEPRESENTER_VERSION "1.0.0"
#endif

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &UpdateChecker::onResult);
}

void UpdateChecker::checkForUpdates(const QString &url)
{
    QUrl qurl(url);
    if (!qurl.isValid()) {
        emit checkFailed("Invalid update URL");
        return;
    }

    QNetworkRequest request(qurl);
    // Follow redirects (e.g. for GitHub raw user content)
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    manager->get(request);
}

void UpdateChecker::onResult(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit checkFailed(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit checkFailed("Invalid update information format");
        return;
    }

    QJsonObject obj = doc.object();
    QString latestVersionStr = obj["latest_version"].toString();
    QString downloadUrl = obj["download_url"].toString();
    QString releaseNotes = obj["release_notes"].toString();

    if (latestVersionStr.isEmpty()) {
        emit checkFailed("Missing version information in update file");
        return;
    }

    QVersionNumber currentVersion = QVersionNumber::fromString(QStringLiteral(SIMPLEPRESENTER_VERSION));
    QVersionNumber latestVersion = QVersionNumber::fromString(latestVersionStr);

    // Only notify if the remote version is strictly greater than current
    if (latestVersion > currentVersion) {
        emit updateAvailable(latestVersionStr, downloadUrl, releaseNotes);
    } else {
        emit noUpdateAvailable();
    }
}
