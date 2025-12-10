#ifndef STREAMENCODER_H
#define STREAMENCODER_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QThread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class StreamEncoder : public QObject
{
    Q_OBJECT

public:
    explicit StreamEncoder(QObject *parent = nullptr);
    ~StreamEncoder();
    
    // Streaming control
    bool startStreaming();
    void stopStreaming();
    bool isStreaming() const { return streaming; }
    
    // Encode and send frame
    void encodeFrame(const QImage &frame);
    
    // Settings
    void setRtmpUrl(const QString &url);
    void setStreamKey(const QString &key);
    void setResolution(int width, int height);
    void setBitrate(int bitrate);
    void setFramerate(int fps);
    
    QString getRtmpUrl() const { return rtmpUrl; }
    QString getStreamKey() const { return streamKey; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getBitrate() const { return bitrate; }
    int getFramerate() const { return framerate; }

signals:
    void streamingStatusChanged(bool streaming);
    void encodingError(const QString &error);

private:
    bool initializeEncoder();
    void cleanupEncoder();
    
    bool streaming;
    QString rtmpUrl;
    QString streamKey;
    int width;
    int height;
    int bitrate;
    int framerate;
    
    // FFmpeg context
    AVFormatContext *formatContext;
    AVCodecContext *codecContext;
    AVStream *videoStream;
    SwsContext *swsContext;
    AVFrame *frame;
    AVPacket *packet;
    int64_t frameCount;
};

#endif // STREAMENCODER_H
