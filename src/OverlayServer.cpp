#include "OverlayServer.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QTcpSocket>
#include <QIcon>
#include <QImage>
#include <QBuffer>

namespace {

QString escapeJsonString(QString value)
{
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    value.replace("\n", "\\n");
    value.replace("\r", "");
    return value;
}

QString formatBracketTextPlain(const QString &input)
{
    QString output;
    output.reserve(input.size());
    for (const QChar &ch : input) {
        if (ch == '[' || ch == ']') {
            continue;
        }
        output.append(ch);
    }
    return output;
}

QString formatBracketTextHtml(const QString &input)
{
    QString result;
    QString buffer;
    bool italic = false;
    auto flushBuffer = [&]() {
        if (!buffer.isEmpty()) {
            result += buffer.toHtmlEscaped();
            buffer.clear();
        }
    };

    for (const QChar &ch : input) {
        if (ch == '[') {
            flushBuffer();
            if (!italic) {
                result += "<span class=\"bracket-text\">";
                italic = true;
            }
            continue;
        }
        if (ch == ']') {
            flushBuffer();
            if (italic) {
                result += "</span>";
                italic = false;
            }
            continue;
        }
        buffer.append(ch);
    }

    flushBuffer();
    if (italic) {
        result += "</span>";
    }

    return result;
}

} // namespace

OverlayServer::OverlayServer(QObject *parent)
    : QObject(parent)
    , server(new QTcpServer(this))
    , wsServer(new QWebSocketServer("OverlayWebSocket", QWebSocketServer::NonSecureMode, this))
    , notesVisible(false)
    , currentMediaIsVideo(false)
    , currentMediaTimestamp(0)
    , refreshTimestamp(0)
    , canvasWidth(1920)
    , canvasHeight(1080)
    , textColor(Qt::white)
    , referenceColor(Qt::white)
    , textBold(true)
    , textItalic(false)
    , textUnderline(false)
    , textUppercase(false)
    , textShadow(true)
    , textOutline(false)
    , textOutlineWidth(2)
    , textOutlineColor(Qt::black)
    , textHighlight(false)
    , textHighlightColor(0, 0, 0, 180)
    , refBold(false)
    , refItalic(true)
    , refUnderline(false)
    , refUppercase(false)
    , refShadow(true)
    , refOutline(false)
    , refOutlineWidth(2)
    , refOutlineColor(Qt::black)
    , refHighlight(false)
    , refHighlightColor(0, 0, 0, 180)
    , notesImageTimestamp(0)
{
    textFont.setFamily("Arial");
    textFont.setPointSize(42);
    
    referenceFont.setFamily("Arial");
    referenceFont.setPointSize(28);
    
    loadSettings();
    
    connect(server, &QTcpServer::newConnection, this, &OverlayServer::onNewConnection);
    connect(wsServer, &QWebSocketServer::newConnection, this, &OverlayServer::onWsNewConnection);
}

OverlayServer::~OverlayServer()
{
    stop();
}

bool OverlayServer::start(quint16 port)
{
    if (server->isListening()) {
        return true;
    }
    
    bool httpOk = server->listen(QHostAddress::Any, port);
    bool wsOk = wsServer->listen(QHostAddress::Any, port + 1);  // WebSocket on port 8081
    
    return httpOk && wsOk;
}

void OverlayServer::stop()
{
    for (QTcpSocket *client : clients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    clients.clear();
    
    for (QWebSocket *wsClient : wsClients) {
        wsClient->close();
        wsClient->deleteLater();
    }
    wsClients.clear();
    
    if (server->isListening()) {
        server->close();
    }
    
    if (wsServer->isListening()) {
        wsServer->close();
    }
}

bool OverlayServer::isRunning() const
{
    return server->isListening();
}

quint16 OverlayServer::port() const
{
    return server->serverPort();
}

void OverlayServer::updateOverlay(const QString &reference, const QString &text)
{
    currentReference = reference;
    currentText = text;
}

void OverlayServer::clearOverlay()
{
    currentReference.clear();
    currentText.clear();
    currentNotesHtml.clear();
    notesVisible = false;
    currentNotesPng.clear();
    notesImageTimestamp = 0;
    currentMediaPath.clear();
    currentMediaIsVideo = false;
    currentMediaTimestamp = QDateTime::currentMSecsSinceEpoch();
    currentYouTubeUrl.clear();
}

void OverlayServer::updateNotes(const QString &notesHtml, bool visible)
{
    currentNotesHtml = notesHtml;
    notesVisible = visible;
}

void OverlayServer::updateNotesImage(const QImage &image, bool visible)
{
    notesVisible = visible;
    currentNotesPng.clear();

    if (visible && !image.isNull()) {
        QImage img = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

        // The notes image comes from the Projection Canvas preview. To avoid
        // the OBS overlay making the text look larger than it appears on the
        // canvas, never upscale it beyond the size of the media card. If the
        // preview image is larger than the card, gently scale it down to fit;
        // otherwise, keep it at its original size.
        if (canvasWidth > 0 && canvasHeight > 0) {
            const int cardW = static_cast<int>(canvasWidth * 0.40);
            const int cardH = static_cast<int>(canvasHeight * 0.40);
            if (cardW > 0 && cardH > 0 && (img.width() > cardW || img.height() > cardH)) {
                img = img.scaled(cardW, cardH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
        QBuffer buffer(&currentNotesPng);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "PNG");
        notesImageTimestamp = QDateTime::currentMSecsSinceEpoch();
    } else {
        notesImageTimestamp = 0;
    }
}

void OverlayServer::updateMedia(const QString &mediaPath, bool isVideo)
{
    // Always update timestamp so the overlay reloads even if the same file path is reused
    currentMediaTimestamp = QDateTime::currentMSecsSinceEpoch();
    currentMediaPath = mediaPath;
    currentMediaIsVideo = isVideo;
}

void OverlayServer::updateYouTube(const QString &youtubeUrl)
{
    currentYouTubeUrl = youtubeUrl;
}

void OverlayServer::triggerRefresh()
{
    refreshTimestamp = QDateTime::currentMSecsSinceEpoch();
}

void OverlayServer::broadcastMediaSeek(qint64 positionMs)
{
    QJsonObject msg;
    msg["type"] = "media_seek";
    msg["positionMs"] = positionMs;
    msg["timestamp"] = currentMediaTimestamp;
    
    QJsonDocument doc(msg);
    QString json = doc.toJson(QJsonDocument::Compact);
    
    for (QWebSocket *wsClient : wsClients) {
        wsClient->sendTextMessage(json);
    }
}

void OverlayServer::broadcastYouTubeSeek(double ratio)
{
    QJsonObject msg;
    msg["type"] = "youtube_seek";
    msg["ratio"] = ratio;

    QJsonDocument doc(msg);
    QString json = doc.toJson(QJsonDocument::Compact);

    for (QWebSocket *wsClient : wsClients) {
        wsClient->sendTextMessage(json);
    }
}

void OverlayServer::broadcastYouTubePlayPause(bool playing)
{
    QJsonObject msg;
    msg["type"] = "youtube_playpause";
    msg["playing"] = playing;

    QJsonDocument doc(msg);
    QString json = doc.toJson(QJsonDocument::Compact);

    for (QWebSocket *wsClient : wsClients) {
        wsClient->sendTextMessage(json);
    }
}

void OverlayServer::broadcastMediaPlayPause(bool playing)
{
    QJsonObject msg;
    msg["type"] = "media_playpause";
    msg["playing"] = playing;
    msg["timestamp"] = currentMediaTimestamp;
    
    QJsonDocument doc(msg);
    QString json = doc.toJson(QJsonDocument::Compact);
    
    for (QWebSocket *wsClient : wsClients) {
        wsClient->sendTextMessage(json);
    }
}

void OverlayServer::broadcastMediaLoad(const QString &mediaPath, bool isVideo, qint64 timestamp)
{
    QJsonObject msg;
    msg["type"] = "media_load";
    msg["isVideo"] = isVideo;
    msg["timestamp"] = timestamp;
    
    QJsonDocument doc(msg);
    QString json = doc.toJson(QJsonDocument::Compact);
    
    for (QWebSocket *wsClient : wsClients) {
        wsClient->sendTextMessage(json);
    }
}

void OverlayServer::setTextColor(const QColor &color)
{
    textColor = color;
}

void OverlayServer::setReferenceColor(const QColor &color)
{
    referenceColor = color;
}

void OverlayServer::setFont(const QFont &font)
{
    textFont = font;
    textBold = font.bold();
    textItalic = font.italic();
    textUnderline = font.underline();
}

void OverlayServer::setReferenceFont(const QFont &refFont)
{
    referenceFont = refFont;
    refBold = refFont.bold();
    refItalic = refFont.italic();
    refUnderline = refFont.underline();
}

void OverlayServer::loadSettings()
{
    QSettings settings("SimplePresenter", "SimplePresenter");
    settings.beginGroup("OBSOverlay");
    
    // Load canvas resolution
    canvasWidth = settings.value("canvasWidth", 1920).toInt();
    canvasHeight = settings.value("canvasHeight", 1080).toInt();
    
    QString textFontFamily = settings.value("textFont", "Arial").toString();
    textFont.setFamily(textFontFamily);
    textFont.setPointSize(settings.value("textFontSize", 42).toInt());
    textColor = settings.value("textColor", QColor(Qt::white)).value<QColor>();
    textBold = settings.value("textBold", true).toBool();
    textItalic = settings.value("textItalic", false).toBool();
    textUnderline = settings.value("textUnderline", false).toBool();
    textUppercase = settings.value("textUppercase", false).toBool();
    textShadow = settings.value("textShadow", true).toBool();
    textOutline = settings.value("textOutline", false).toBool();
    textOutlineWidth = settings.value("textOutlineWidth", 2).toInt();
    textOutlineColor = settings.value("textOutlineColor", QColor(Qt::black)).value<QColor>();
    textHighlight = settings.value("textHighlight", false).toBool();
    textHighlightColor = settings.value("textHighlightColor", QColor(0, 0, 0, 180)).value<QColor>();
    textFont.setBold(textBold);
    textFont.setItalic(textItalic);
    textFont.setUnderline(textUnderline);
    
    QString refFontFamily = settings.value("refFont", "Arial").toString();
    referenceFont.setFamily(refFontFamily);
    referenceFont.setPointSize(settings.value("refFontSize", 28).toInt());
    referenceColor = settings.value("refColor", QColor(Qt::white)).value<QColor>();
    refBold = settings.value("refBold", false).toBool();
    refItalic = settings.value("refItalic", true).toBool();
    refUnderline = settings.value("refUnderline", false).toBool();
    refUppercase = settings.value("refUppercase", false).toBool();
    refShadow = settings.value("refShadow", true).toBool();
    refOutline = settings.value("refOutline", false).toBool();
    refOutlineWidth = settings.value("refOutlineWidth", 2).toInt();
    refOutlineColor = settings.value("refOutlineColor", QColor(Qt::black)).value<QColor>();
    refHighlight = settings.value("refHighlight", false).toBool();
    refHighlightColor = settings.value("refHighlightColor", QColor(0, 0, 0, 180)).value<QColor>();
    referenceFont.setBold(refBold);
    referenceFont.setItalic(refItalic);
    referenceFont.setUnderline(refUnderline);
    
    settings.endGroup();
}

void OverlayServer::onNewConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket *client = server->nextPendingConnection();
        clients.append(client);
        
        connect(client, &QTcpSocket::disconnected, this, &OverlayServer::onClientDisconnected);
        connect(client, &QTcpSocket::readyRead, this, &OverlayServer::onReadyRead);
    }
}

void OverlayServer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        clients.removeAll(client);
        client->deleteLater();
    }
}

void OverlayServer::onWsNewConnection()
{
    QWebSocket *wsClient = wsServer->nextPendingConnection();
    if (wsClient) {
        wsClients.append(wsClient);
        connect(wsClient, &QWebSocket::disconnected, this, &OverlayServer::onWsDisconnected);
        qDebug() << "WebSocket client connected";
        emit webSocketClientConnected();
    }
}

void OverlayServer::onWsDisconnected()
{
    QWebSocket *wsClient = qobject_cast<QWebSocket*>(sender());
    if (wsClient) {
        wsClients.removeAll(wsClient);
        wsClient->deleteLater();
        qDebug() << "WebSocket client disconnected";
    }
}

void OverlayServer::onReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        return;
    }
    
    QString request = QString::fromUtf8(client->readAll());
    QStringList lines = request.split("\r\n");
    
    if (lines.isEmpty()) {
        sendNotFound(client);
        return;
    }
    
    QStringList requestLine = lines[0].split(" ");
    if (requestLine.size() < 2) {
        sendNotFound(client);
        return;
    }
    
    QString method = requestLine[0];
    QString path = requestLine[1];
    
    if (method == "GET") {
        if (path == "/" || path == "/overlay") {
            sendResponse(client, generateHTML(), "text/html; charset=utf-8");
        } else if (path.startsWith("/media")) {
            // Serve the current media file
            if (currentMediaPath.isEmpty()) {
                // No media to serve - return empty response instead of 404
                QString response = 
                    "HTTP/1.1 204 No Content\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Connection: close\r\n"
                    "\r\n";
                client->write(response.toUtf8());
                client->flush();
                client->disconnectFromHost();
                return;
            }
            
            if (!QFile::exists(currentMediaPath)) {
                qWarning() << "Media file does not exist:" << currentMediaPath;
                sendNotFound(client);
                return;
            }
            
            QFile file(currentMediaPath);
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "Failed to open media file:" << currentMediaPath;
                sendNotFound(client);
                return;
            }
            
            QByteArray data = file.readAll();
            file.close();
            
            QString contentType;
            if (currentMediaIsVideo) {
                QString ext = QFileInfo(currentMediaPath).suffix().toLower();
                if (ext == "mp4") contentType = "video/mp4";
                else if (ext == "webm") contentType = "video/webm";
                else if (ext == "avi") contentType = "video/x-msvideo";
                else if (ext == "mov") contentType = "video/quicktime";
                else contentType = "video/mp4";
            } else {
                QString ext = QFileInfo(currentMediaPath).suffix().toLower();
                if (ext == "jpg" || ext == "jpeg") contentType = "image/jpeg";
                else if (ext == "png") contentType = "image/png";
                else if (ext == "gif") contentType = "image/gif";
                else if (ext == "bmp") contentType = "image/bmp";
                else if (ext == "webp") contentType = "image/webp";
                else contentType = "image/jpeg";
            }
            
            QString response = QString(
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %1\r\n"
                "Content-Length: %2\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Cache-Control: no-cache\r\n"
                "Accept-Ranges: bytes\r\n"
                "Connection: close\r\n"
                "\r\n"
            ).arg(contentType).arg(data.size());
            
            client->write(response.toUtf8());
            client->write(data);
            client->flush();
            client->disconnectFromHost();
        } else if (path.startsWith("/notes")) {
            // Serve the current notes PNG (transparent text overlay)
            if (currentNotesPng.isEmpty() || !notesVisible || notesImageTimestamp == 0) {
                QString response =
                    "HTTP/1.1 204 No Content\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Connection: close\r\n"
                    "\r\n";
                client->write(response.toUtf8());
                client->flush();
                client->disconnectFromHost();
            } else {
                QString response = QString(
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: image/png\r\n"
                    "Content-Length: %1\r\n"
                    "Access-Control-Allow-Origin: *\r\n"
                    "Cache-Control: no-cache\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                ).arg(currentNotesPng.size());

                client->write(response.toUtf8());
                client->write(currentNotesPng);
                client->flush();
                client->disconnectFromHost();
            }
        } else if (path == "/data") {
            // JSON endpoint for polling
            QString escapedRef = currentReference;
            escapedRef.replace("\\", "\\\\");
            escapedRef.replace("\"", "\\\"");
            
            QString plainText = formatBracketTextPlain(currentText);
            QString htmlText = formatBracketTextHtml(currentText);

            QString escapedText = escapeJsonString(plainText);
            QString escapedTextHtml = escapeJsonString(htmlText);

            // Notes HTML is still sent for compatibility, but OBS now
            // primarily uses the notes PNG image and timestamp.
            QString escapedNotesHtml = escapeJsonString(currentNotesHtml);

            // Only send a flag indicating media presence, not the full path
            bool hasMedia = !currentMediaPath.isEmpty();
            bool hasYouTube = !currentYouTubeUrl.isEmpty();
            QString escapedYouTube = escapeJsonString(currentYouTubeUrl);
            
            QString json = QString(
                "{\"reference\":\"%1\",\"text\":\"%2\",\"textHtml\":\"%3\"," 
                "\"hasMedia\":%4,\"mediaIsVideo\":%5,\"mediaTimestamp\":%6,\"refreshTimestamp\":%7," 
                "\"hasYouTube\":%8,\"youTubeUrl\":\"%9\",\"notesVisible\":%10,\"notesHtml\":\"%11\",\"notesImageTimestamp\":%12}")
                .arg(escapedRef)
                .arg(escapedText)
                .arg(escapedTextHtml)
                .arg(hasMedia ? "true" : "false")
                .arg(currentMediaIsVideo ? "true" : "false")
                .arg(currentMediaTimestamp)
                .arg(refreshTimestamp)
                .arg(hasYouTube ? "true" : "false")
                .arg(escapedYouTube)
                .arg(notesVisible ? "true" : "false")
                .arg(escapedNotesHtml)
                .arg(notesImageTimestamp);
            sendResponse(client, json, "application/json");
        } else {
            sendNotFound(client);
        }
    } else {
        sendNotFound(client);
    }
    
    client->disconnectFromHost();
}

void OverlayServer::sendResponse(QTcpSocket *socket, const QString &content, const QString &contentType)
{
    QByteArray data = content.toUtf8();
    QString response = QString(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %1\r\n"
        "Content-Length: %2\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: close\r\n"
        "\r\n"
    ).arg(contentType).arg(data.size());
    
    socket->write(response.toUtf8());
    socket->write(data);
    socket->flush();
}

void OverlayServer::sendNotFound(QTcpSocket *socket)
{
    QString response = 
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 9\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Not Found";
    
    socket->write(response.toUtf8());
    socket->flush();
}

QString OverlayServer::generateHTML() const
{
    QString fontFamily = textFont.family();
    int fontSize = textFont.pointSize();
    QString fontWeight = textBold ? "bold" : "normal";
    QString fontStyle = textItalic ? "italic" : "normal";
    QString textShadowStr = textShadow ? "2px 2px 6px rgba(0,0,0,0.9)" : "none";
    
    QString textStrokeStr = textOutline 
        ? QString("-webkit-text-stroke: %1px rgba(%2,%3,%4,1);")
            .arg(textOutlineWidth)
            .arg(textOutlineColor.red())
            .arg(textOutlineColor.green())
            .arg(textOutlineColor.blue())
        : "";
    
    QString textBgColor = textHighlight 
        ? QString("rgba(%1,%2,%3,%4)")
            .arg(textHighlightColor.red())
            .arg(textHighlightColor.green())
            .arg(textHighlightColor.blue())
            .arg(textHighlightColor.alpha() / 255.0, 0, 'f', 2)
        : "transparent";
    
    QString refFontFamily = referenceFont.family();
    int refFontSize = referenceFont.pointSize();
    QString refFontWeight = refBold ? "bold" : "normal";
    QString refFontStyle = refItalic ? "italic" : "normal";
    QString textDecoration = textUnderline ? "underline" : "none";
    QString refDecoration = refUnderline ? "underline" : "none";
    QString refShadowStr = refShadow ? "2px 2px 4px rgba(0,0,0,0.8)" : "none";
    
    QString refStrokeStr = refOutline 
        ? QString("-webkit-text-stroke: %1px rgba(%2,%3,%4,1);")
            .arg(refOutlineWidth)
            .arg(refOutlineColor.red())
            .arg(refOutlineColor.green())
            .arg(refOutlineColor.blue())
        : "";
    
    QString refBgColor = refHighlight
        ? QString("rgba(%1,%2,%3,%4)")
            .arg(refHighlightColor.red())
            .arg(refHighlightColor.green())
            .arg(refHighlightColor.blue())
            .arg(refHighlightColor.alpha() / 255.0, 0, 'f', 2)
        : "transparent";
    QString textTransform = textUppercase ? "uppercase" : "none";
    QString refTransform = refUppercase ? "uppercase" : "none";

    QString textColorStr = textColor.name();
    QString refColorStr = referenceColor.name();
    
    QString html = QString(R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=1920, height=1080">
    <meta name="referrer" content="no-referrer-when-downgrade">
    <title>Overlay</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            width: %15px;
            height: %16px;
            background: transparent;
            overflow: hidden;
            display: flex;
            align-items: flex-end;
            justify-content: center;
            font-family: '%1', Arial, sans-serif;
            padding-bottom: 50px;
            position: relative;
        }
        #media-container {
            position: absolute;
            bottom: 5%;
            left: 50%;
            transform: translateX(-50%);
            width: 40%;
            height: 40%;
            background: rgba(0, 0, 0, 0.85);
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-radius: 12px;
            overflow: hidden;
            z-index: 100;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.6);
        }
        #media-background, #media-background-video, #media-youtube {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            object-fit: contain;
        }
        #container {
            width: 100%%;
            padding: 0 120px;
            opacity: 0;
            transform: translateY(30px);
            transition: opacity 0.6s ease-out, transform 0.6s ease-out;
            position: relative;
            z-index: 10;
        }
        #container.visible {
            opacity: 1;
            transform: translateY(0);
        }
        #content-wrapper {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            width: 100%%;
        }
        #text-row {
            display: flex;
            align-items: flex-start;
            justify-content: center;
            gap: 12px;
            background-color: %13;
            padding: 16px 60px;
            border-radius: 4px;
            margin-bottom: 12px;
            box-sizing: border-box;
            margin-left: 40px;
            margin-right: 40px;
        }
        #verse-number {
            font-family: '%1', Arial, sans-serif;
            font-size: %2pt;
            font-weight: %3;
            color: #4A9EFF;
            text-shadow: %4;
            %17
            flex-shrink: 0;
            padding-top: 2px;
        }
        #text {
            font-family: '%1', Arial, sans-serif;
            font-size: %2pt;
            font-weight: %3;
            font-style: %5;
            color: %6;
            line-height: 1.5;
            text-shadow: %4;
            %17
            text-decoration: %19;
            text-transform: %21;
            white-space: pre-wrap;
            flex: 1;
            text-align: center;
        }
        .bracket-text {
            font-style: italic;
        }
        #reference {
            font-family: '%7', Arial, sans-serif;
            font-size: %8pt;
            font-weight: %9;
            font-style: %10;
            color: %11;
            text-shadow: %12;
            %18
            text-decoration: %20;
            text-transform: %22;
            letter-spacing: 1px;
            opacity: 0.95;
            background-color: %14;
            padding: 8px 20px;
            border-radius: 4px;
            display: block;
            width: 70%%;
            max-width: 70%%;
            margin: 0 auto;
            text-align: center;
        }
        #notes-overlay {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%%;
            height: 100%%;
            display: none;
            z-index: 150;
            pointer-events: none;
        }
        #notes-image {
            position: absolute;
            top: 0;
            left: 0;
            max-width: 100%%;
            max-height: 100%%;
            width: auto;
            height: auto;
            object-fit: contain;
            object-position: top left;
        }
    </style>
</head>
<body>
    <div id="media-container" style="display:none;">
        <img id="media-background" style="display:none;" />
        <video id="media-background-video" style="display:none;" loop muted autoplay></video>
        <iframe id="media-youtube" style="display:none;" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" allowfullscreen></iframe>
        <div id="notes-overlay">
            <img id="notes-image" alt="" />
        </div>
    </div>
    <div id="container">
        <div id="content-wrapper">
            <div id="text-row">
                <div id="verse-number"></div>
                <div id="text"></div>
            </div>
            <div id="reference"></div>
        </div>
    </div>
    <script src="https://www.youtube.com/iframe_api"></script>
    <script>
        let lastReference = '';
        let lastText = '';
        let lastHasMedia = false;
        let lastMediaTimestamp = 0;
        let lastYouTubeUrl = '';
        let lastRefreshTimestamp = 0;
        let lastNotesImageTimestamp = 0;
        let isVisible = false;
        let ytPlayer = null;
        let pendingYouTubeUrl = null;
        let ytErrorActive = false;
        
        // WebSocket for video sync
        let ws = null;
        let wsReconnectTimer = null;
        
        function connectWebSocket() {
            try {
                ws = new WebSocket('ws://localhost:8081');
                
                ws.onopen = () => {
                    console.log('WebSocket connected');
                    if (wsReconnectTimer) {
                        clearTimeout(wsReconnectTimer);
                        wsReconnectTimer = null;
                    }
                };
                
                ws.onmessage = (event) => {
                    try {
                        const msg = JSON.parse(event.data);
                        handleWebSocketMessage(msg);
                    } catch (e) {
                        console.error('WebSocket message parse error:', e);
                    }
                };
                
                ws.onerror = (error) => {
                    console.error('WebSocket error:', error);
                };
                
                ws.onclose = () => {
                    console.log('WebSocket disconnected, reconnecting in 2s...');
                    ws = null;
                    wsReconnectTimer = setTimeout(connectWebSocket, 2000);
                };
            } catch (e) {
                console.error('WebSocket connection error:', e);
                wsReconnectTimer = setTimeout(connectWebSocket, 2000);
            }
        }
        
        function handleWebSocketMessage(msg) {
            const vidBg = document.getElementById('media-background-video');
            const ytFrame = document.getElementById('media-youtube');
            
            if (msg.type === 'media_seek' && msg.timestamp === lastMediaTimestamp) {
                // Only seek if this is the current media
                if (vidBg && vidBg.style.display === 'block') {
                    vidBg.currentTime = msg.positionMs / 1000;
                    console.log('Synced video to:', msg.positionMs, 'ms');
                }
            } else if (msg.type === 'media_playpause' && msg.timestamp === lastMediaTimestamp) {
                if (vidBg && vidBg.style.display === 'block') {
                    if (msg.playing) {
                        vidBg.play().catch(e => console.log('Play failed:', e));
                    } else {
                        vidBg.pause();
                    }
                    console.log('Video', msg.playing ? 'playing' : 'paused');
                }
            } else if (msg.type === 'youtube_seek') {
                try {
                    if (ytPlayer && ytFrame && ytFrame.style.display === 'block' &&
                        typeof ytPlayer.getDuration === 'function' &&
                        typeof ytPlayer.seekTo === 'function') {
                        let r = msg.ratio;
                        if (typeof r !== 'number' || isNaN(r)) {
                            return;
                        }
                        if (r < 0) r = 0;
                        if (r > 1) r = 1;
                        const d = ytPlayer.getDuration();
                        if (!d || d <= 0) {
                            return;
                        }
                        ytPlayer.seekTo(d * r, true);
                        console.log('YouTube seek ratio:', r);
                    }
                } catch (e) {
                    console.error('YouTube seek via WebSocket failed:', e);
                }
            } else if (msg.type === 'youtube_playpause') {
                try {
                    if (ytPlayer && ytFrame && ytFrame.style.display === 'block') {
                        if (msg.playing) {
                            if (typeof ytPlayer.playVideo === 'function') {
                                ytPlayer.playVideo();
                            }
                        } else {
                            if (typeof ytPlayer.pauseVideo === 'function') {
                                ytPlayer.pauseVideo();
                            }
                        }
                        console.log('YouTube', msg.playing ? 'play' : 'pause');
                    }
                } catch (e) {
                    console.error('YouTube play/pause via WebSocket failed:', e);
                }
            } else if (msg.type === 'media_load') {
                console.log('Media load event, timestamp:', msg.timestamp);
                // Media will be loaded via HTTP polling
            }
        }

        function onYouTubeIframeAPIReady() {
            try {
                const origin = (window && window.location && window.location.origin)
                    ? window.location.origin
                    : 'http://127.0.0.1:8080';
                ytPlayer = new YT.Player('media-youtube', {
                    width: '100%',
                    height: '100%',
                    host: 'https://www.youtube.com',
                    playerVars: {
                        autoplay: 1,
                        rel: 0,
                        modestbranding: 1,
                        mute: 1,
                        origin: origin,
                        playsinline: 1
                    },
                    events: {
                        'onReady': function () {
                            if (pendingYouTubeUrl) {
                                const url = pendingYouTubeUrl;
                                pendingYouTubeUrl = null;
                                loadYouTubeVideo(url);
                            }
                        },
                        'onError': onYouTubeError
                    }
                });
            } catch (e) {
                console.error('Failed to initialise YouTube IFrame API player:', e);
            }
        }

        function onYouTubeError(event) {
            ytErrorActive = true;
            console.error('YouTube player error:', event && event.data);
        }

        function loadYouTubeVideo(url) {
            const ytFrame = document.getElementById('media-youtube');
            if (!url) {
                return;
            }

            // Prefer the IFrame API when available
            if (typeof YT !== 'undefined' && YT.Player && ytPlayer && typeof ytPlayer.loadVideoById === 'function') {
                try {
                    const u = new URL(url);
                    let videoId = '';
                    let start = '';

                    if (u.hostname.includes('youtu.be')) {
                        videoId = u.pathname.replace('/', '');
                        start = u.searchParams.get('t') || '';
                    } else if (u.hostname.includes('youtube.com')) {
                        if (u.pathname.startsWith('/embed/')) {
                            videoId = u.pathname.replace('/embed/', '');
                            start = u.searchParams.get('start') || '';
                        } else if (u.pathname === '/watch') {
                            videoId = u.searchParams.get('v') || '';
                            start = u.searchParams.get('t') || '';
                        }
                    }

                    if (!videoId) {
                        // Fall back to static embed if parsing failed
                        const embedUrl = buildYouTubeEmbed(url);
                        if (ytFrame) {
                            ytFrame.src = embedUrl;
                        }
                        return;
                    }

                    const startSeconds = start ? (parseInt(start, 10) || 0) : 0;
                    if (startSeconds > 0) {
                        ytPlayer.loadVideoById({ videoId: videoId, startSeconds: startSeconds });
                    } else {
                        ytPlayer.loadVideoById(videoId);
                    }
                    if (typeof ytPlayer.mute === 'function') {
                        ytPlayer.mute();
                    }
                } catch (e) {
                    console.error('Failed to load YouTube video via API, falling back to src:', e);
                    const embedUrl = buildYouTubeEmbed(url);
                    if (ytFrame) {
                        ytFrame.src = embedUrl;
                    }
                }
            } else {
                // API not ready yet: remember URL and also set static src as fallback
                pendingYouTubeUrl = url;
                const embedUrl = buildYouTubeEmbed(url);
                if (ytFrame) {
                    ytFrame.src = embedUrl;
                }
            }
        }

        // Connect WebSocket on load
        connectWebSocket();
        
)" R"(
        function extractVerseNumber(ref) {
            const match = ref.match(/(\d+)$/);
            return match ? match[1] : '';
        }
        
        function buildYouTubeEmbed(url) {
            try {
                const u = new URL(url);
                let videoId = '';
                let start = '';

                if (u.hostname.includes('youtu.be')) {
                    videoId = u.pathname.replace('/', '');
                    start = u.searchParams.get('t') || '';
                } else if (u.hostname.includes('youtube.com')) {
                    if (u.pathname.startsWith('/embed/')) {
                        // Already an embed URL, just return it as-is.
                        return url;
                    }
                    if (u.pathname === '/watch') {
                        videoId = u.searchParams.get('v') || '';
                        start = u.searchParams.get('t') || '';
                    }
                }

                if (!videoId) {
                    return url;
                }

                const params = new URLSearchParams();
                if (start) {
                    params.set('start', start);
                }
                params.set('autoplay', '1');
                params.set('rel', '0');
                params.set('modestbranding', '1');
                // Mute audio in the OBS overlay; main audio should come from the projection output
                params.set('mute', '1');

                // Provide an explicit origin and enable JS API so the player can be
                // controlled when needed.
                const origin = (window && window.location && window.location.origin)
                    ? window.location.origin
                    : 'http://127.0.0.1:8080';
                params.set('origin', origin);
                params.set('enablejsapi', '1');
                params.set('playsinline', '1');

                return 'https://www.youtube.com/embed/' + videoId + '?' + params.toString();
            } catch (e) {
                console.error('Failed to build YouTube embed URL', e);
                return url;
            }
        }
        
        function updateOverlay() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    // Check if settings changed and need to refresh
                    if (data.refreshTimestamp !== lastRefreshTimestamp && lastRefreshTimestamp !== 0) {
                        console.log('Settings changed, refreshing page...');
                        location.reload();
                        return;
                    }
                    lastRefreshTimestamp = data.refreshTimestamp;

                    const hasContent = (!data.notesVisible) && (data.reference || data.text);

                    // Update media / YouTube background
                    const mediaContainer = document.getElementById('media-container');
                    const imgBg = document.getElementById('media-background');
                    const vidBg = document.getElementById('media-background-video');
                    const ytFrame = document.getElementById('media-youtube');

                    const hasYouTube = data.hasYouTube && data.youTubeUrl;

                    if (hasYouTube && data.youTubeUrl !== lastYouTubeUrl) {
                        // New YouTube video requested: clear any previous error
                        // state and try to load this video.
                        lastYouTubeUrl = data.youTubeUrl;
                        ytErrorActive = false;
                        const overlayContainer = document.getElementById('container');
                        if (overlayContainer) {
                            overlayContainer.style.display = '';
                        }
                        // Use the IFrame API-driven loader when available, with
                        // an internal fallback to setting iframe src directly.
                        loadYouTubeVideo(lastYouTubeUrl);
                    }

                    if (hasYouTube && ytErrorActive) {
                        mediaContainer.style.display = 'none';
                        ytFrame.style.display = 'none';
                        imgBg.style.display = 'none';
                        vidBg.style.display = 'none';
                        const overlayContainer = document.getElementById('container');
                        if (overlayContainer) {
                            overlayContainer.style.display = 'none';
                        }
                        return;
                    }

                    if (hasYouTube) {
                        mediaContainer.style.display = 'block';
                        ytFrame.style.display = 'block';
                        imgBg.style.display = 'none';
                        vidBg.style.display = 'none';
                        vidBg.pause();
                    } else {
                        ytFrame.style.display = 'none';
                        ytFrame.src = '';
                        ytErrorActive = false;
                        // Clear lastYouTubeUrl so that if the same YouTube URL
                        // is projected again later (after showing local media
                        // or plain text), the overlay will treat it as a new
                        // video and reload it.
                        lastYouTubeUrl = '';
                        const overlayContainer = document.getElementById('container');
                        if (overlayContainer) {
                            overlayContainer.style.display = '';
                        }

                        if (data.hasMedia !== lastHasMedia || data.mediaTimestamp !== lastMediaTimestamp) {
                            console.log('Media update - hasMedia:', data.hasMedia, 'isVideo:', data.mediaIsVideo, 'timestamp:', data.mediaTimestamp);
                            lastHasMedia = data.hasMedia;
                            lastMediaTimestamp = data.mediaTimestamp;

                            if (data.hasMedia) {
                                const mediaUrl = '/media?t=' + data.mediaTimestamp;
                                console.log('Loading media from:', mediaUrl);

                                // Clear all error handlers first
                                imgBg.onerror = null;
                                imgBg.onload = null;
                                vidBg.onerror = null;

                                if (data.mediaIsVideo) {
                                    imgBg.style.display = 'none';
                                    imgBg.src = '';
                                    vidBg.onerror = (e) => console.error('Video load error:', e);
                                    vidBg.onloadeddata = () => console.log('Video loaded successfully');
                                    vidBg.src = mediaUrl;
                                    vidBg.style.display = 'block';
                                    vidBg.load();
                                    vidBg.play().catch(e => console.log('Video play failed:', e));
                                } else {
                                    vidBg.style.display = 'none';
                                    vidBg.pause();
                                    vidBg.src = '';
                                    imgBg.onerror = (e) => console.error('Image load error:', e);
                                    imgBg.onload = () => console.log('Image loaded successfully');
                                    imgBg.src = mediaUrl;
                                    imgBg.style.display = 'block';
                                }
                                mediaContainer.style.display = 'block';
                            } else {
                                mediaContainer.style.display = 'none';
                                imgBg.style.display = 'none';
                                vidBg.style.display = 'none';
                                vidBg.pause();
                                imgBg.src = '';
                                vidBg.src = '';
                            }
                        }
                    }

                    const container = document.getElementById('container');
                    const textRow = document.getElementById('text-row');
                    const refDiv = document.getElementById('reference');
                    const notesOverlay = document.getElementById('notes-overlay');
                    const notesImage = document.getElementById('notes-image');

                    const notesVisible = !!data.notesVisible;
                    const notesImageTimestamp = data.notesImageTimestamp || 0;

                    if (notesOverlay && notesImage) {
                        if (notesVisible && notesImageTimestamp) {
                            notesOverlay.style.display = 'block';
                            if (lastNotesImageTimestamp !== notesImageTimestamp) {
                                notesImage.src = '/notes?t=' + notesImageTimestamp;
                                lastNotesImageTimestamp = notesImageTimestamp;
                            }
                        } else {
                            notesOverlay.style.display = 'none';
                            notesImage.src = '';
                            lastNotesImageTimestamp = 0;
                        }
                    }

                    if (!notesVisible && hasContent) {
                        const verseNum = extractVerseNumber(data.reference || '');
                        document.getElementById('verse-number').textContent = verseNum;
                        const textDiv = document.getElementById('text');
                        textDiv.innerHTML = data.textHtml || '';
                        refDiv.textContent = data.reference || '';

                        textRow.style.display = data.text ? 'flex' : 'none';
                        refDiv.style.display = data.reference ? 'inline-block' : 'none';

                        if (!isVisible) {
                            container.classList.add('visible');
                            isVisible = true;
                        }
                    } else {
                        if (isVisible) {
                            container.classList.remove('visible');
                            isVisible = false;
                        }

                        textRow.style.display = 'none';
                        refDiv.style.display = 'none';
                    }

                    lastReference = data.reference || '';
                    lastText = data.text || '';
                })
                .catch(err => console.error('Error fetching data:', err));
        }

// Poll every 100ms for updates
setInterval(updateOverlay, 100);
</script>
</body>
</html>
)"    ).arg(fontFamily)
     .arg(fontSize)
     .arg(fontWeight)
     .arg(textShadowStr)
     .arg(fontStyle)
     .arg(textColorStr)
     .arg(refFontFamily)
     .arg(refFontSize)
     .arg(refFontWeight)
     .arg(refFontStyle)
     .arg(refColorStr)
     .arg(refShadowStr)
     .arg(textBgColor)
     .arg(refBgColor)
     .arg(canvasWidth)
     .arg(canvasHeight)
     .arg(textStrokeStr)
     .arg(refStrokeStr)
     .arg(textDecoration)
     .arg(refDecoration)
     .arg(textUppercase ? "uppercase" : "none")
     .arg(refUppercase ? "uppercase" : "none");
    
    return html;
}

QString OverlayServer::generateUpdateScript() const
{
    // Not used anymore - data is fetched via JSON endpoint
    return QString();
}
