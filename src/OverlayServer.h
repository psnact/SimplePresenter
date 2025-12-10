#ifndef OVERLAYSERVER_H
#define OVERLAYSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>
#include <QColor>
#include <QFont>
#include <QImage>

class OverlayServer : public QObject
{
    Q_OBJECT

public:
    explicit OverlayServer(QObject *parent = nullptr);
    ~OverlayServer();

    bool start(quint16 port = 8080);
    void stop();
    bool isRunning() const;
    quint16 port() const;

    void updateOverlay(const QString &reference, const QString &text);
    void clearOverlay();
    void updateMedia(const QString &mediaPath, bool isVideo);
    void updateYouTube(const QString &youtubeUrl);
    void updateNotes(const QString &notesHtml, bool visible);
    void updateNotesImage(const QImage &image, bool visible);
    void triggerRefresh();
    
    // WebSocket methods for video sync
    void broadcastMediaSeek(qint64 positionMs);
    void broadcastMediaPlayPause(bool playing);
    void broadcastMediaLoad(const QString &mediaPath, bool isVideo, qint64 timestamp);
    void broadcastYouTubeSeek(double ratio);
    void broadcastYouTubePlayPause(bool playing);
    
    void setTextColor(const QColor &color);
    void setReferenceColor(const QColor &color);
    void setFont(const QFont &font);
    void setReferenceFont(const QFont &refFont);
    void loadSettings();

signals:
    void webSocketClientConnected();

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();
    
    // WebSocket slots
    void onWsNewConnection();
    void onWsDisconnected();

private:
    void sendResponse(QTcpSocket *socket, const QString &content, const QString &contentType = "text/html");
    void sendNotFound(QTcpSocket *socket);
    QString generateHTML() const;
    QString generateUpdateScript() const;

    QTcpServer *server;
    QList<QTcpSocket*> clients;
    
    QWebSocketServer *wsServer;
    QList<QWebSocket*> wsClients;
    
    QString currentReference;
    QString currentText;
    QString currentNotesHtml;
    QString currentMediaPath;
    QString currentYouTubeUrl;
    bool notesVisible;
    bool currentMediaIsVideo;
    qint64 currentMediaTimestamp;
    qint64 refreshTimestamp;
    int canvasWidth;
    int canvasHeight;
    QColor textColor;
    QColor referenceColor;
    QFont textFont;
    QFont referenceFont;
    bool textBold;
    bool textItalic;
    bool textUnderline;
    bool textUppercase;
    bool textShadow;
    bool textOutline;
    int textOutlineWidth;
    QColor textOutlineColor;
    bool textHighlight;
    QColor textHighlightColor;
    bool refBold;
    bool refItalic;
    bool refUnderline;
    bool refUppercase;
    bool refShadow;
    bool refOutline;
    int refOutlineWidth;
    QColor refOutlineColor;
    bool refHighlight;
    QColor refHighlightColor;

    QByteArray currentNotesPng;
    qint64 notesImageTimestamp;
};

#endif // OVERLAYSERVER_H
