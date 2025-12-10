#include "StreamEncoder.h"
#include <QSettings>

StreamEncoder::StreamEncoder(QObject *parent)
    : QObject(parent)
    , streaming(false)
    , width(1920)
    , height(1080)
    , bitrate(4000000)
    , framerate(30)
    , formatContext(nullptr)
    , codecContext(nullptr)
    , videoStream(nullptr)
    , swsContext(nullptr)
    , frame(nullptr)
    , packet(nullptr)
    , frameCount(0)
{
    // Load settings
    QSettings settings("SimplePresenter", "SimplePresenter");
    settings.beginGroup("Streaming");
    rtmpUrl = settings.value("rtmpUrl", "rtmp://a.rtmp.youtube.com/live2/").toString();
    streamKey = settings.value("streamKey", "").toString();
    width = settings.value("width", 1920).toInt();
    height = settings.value("height", 1080).toInt();
    bitrate = settings.value("bitrate", 4000000).toInt();
    framerate = settings.value("framerate", 30).toInt();
    settings.endGroup();
}

StreamEncoder::~StreamEncoder()
{
    if (streaming) {
        stopStreaming();
    }
    
    // Save settings
    QSettings settings("SimplePresenter", "SimplePresenter");
    settings.beginGroup("Streaming");
    settings.setValue("rtmpUrl", rtmpUrl);
    settings.setValue("streamKey", streamKey);
    settings.setValue("width", width);
    settings.setValue("height", height);
    settings.setValue("bitrate", bitrate);
    settings.setValue("framerate", framerate);
    settings.endGroup();
}

void StreamEncoder::setRtmpUrl(const QString &url)
{
    rtmpUrl = url;
}

void StreamEncoder::setStreamKey(const QString &key)
{
    streamKey = key;
}

void StreamEncoder::setResolution(int w, int h)
{
    width = w;
    height = h;
}

void StreamEncoder::setBitrate(int br)
{
    bitrate = br;
}

void StreamEncoder::setFramerate(int fps)
{
    framerate = fps;
}

bool StreamEncoder::startStreaming()
{
    if (streaming) {
        return true;
    }
    
    if (rtmpUrl.isEmpty() || streamKey.isEmpty()) {
        emit encodingError("RTMP URL and Stream Key must be set");
        return false;
    }
    
    if (!initializeEncoder()) {
        emit encodingError("Failed to initialize encoder");
        return false;
    }
    
    streaming = true;
    frameCount = 0;
    emit streamingStatusChanged(true);
    
    return true;
}

void StreamEncoder::stopStreaming()
{
    if (!streaming) {
        return;
    }
    
    cleanupEncoder();
    
    streaming = false;
    emit streamingStatusChanged(false);
}

bool StreamEncoder::initializeEncoder()
{
    int ret;
    
    // Allocate output format context
    QString fullUrl = rtmpUrl + streamKey;
    ret = avformat_alloc_output_context2(&formatContext, nullptr, "flv", fullUrl.toUtf8().constData());
    if (ret < 0) {
        return false;
    }
    
    // Find H.264 encoder (prefer NVENC if available)
    const AVCodec *codec = avcodec_find_encoder_by_name("h264_nvenc");
    if (!codec) {
        codec = avcodec_find_encoder_by_name("libx264");
    }
    if (!codec) {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (!codec) {
        cleanupEncoder();
        return false;
    }
    
    // Create video stream
    videoStream = avformat_new_stream(formatContext, nullptr);
    if (!videoStream) {
        cleanupEncoder();
        return false;
    }
    
    // Allocate codec context
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        cleanupEncoder();
        return false;
    }
    
    // Set codec parameters
    codecContext->codec_id = codec->id;
    codecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    codecContext->width = width;
    codecContext->height = height;
    codecContext->time_base = AVRational{1, framerate};
    codecContext->framerate = AVRational{framerate, 1};
    codecContext->gop_size = framerate * 2; // 2 seconds
    codecContext->max_b_frames = 2;
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext->bit_rate = bitrate;
    
    // Set codec options
    av_opt_set(codecContext->priv_data, "preset", "fast", 0);
    av_opt_set(codecContext->priv_data, "tune", "zerolatency", 0);
    
    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    // Open codec
    ret = avcodec_open2(codecContext, codec, nullptr);
    if (ret < 0) {
        cleanupEncoder();
        return false;
    }
    
    // Copy codec parameters to stream
    ret = avcodec_parameters_from_context(videoStream->codecpar, codecContext);
    if (ret < 0) {
        cleanupEncoder();
        return false;
    }
    
    videoStream->time_base = codecContext->time_base;
    
    // Allocate frame
    frame = av_frame_alloc();
    if (!frame) {
        cleanupEncoder();
        return false;
    }
    
    frame->format = codecContext->pix_fmt;
    frame->width = codecContext->width;
    frame->height = codecContext->height;
    
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        cleanupEncoder();
        return false;
    }
    
    // Allocate packet
    packet = av_packet_alloc();
    if (!packet) {
        cleanupEncoder();
        return false;
    }
    
    // Initialize SWS context for RGB to YUV conversion
    swsContext = sws_getContext(
        width, height, AV_PIX_FMT_RGB32,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    
    if (!swsContext) {
        cleanupEncoder();
        return false;
    }
    
    // Open output file (RTMP stream)
    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&formatContext->pb, fullUrl.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            cleanupEncoder();
            return false;
        }
    }
    
    // Write header
    ret = avformat_write_header(formatContext, nullptr);
    if (ret < 0) {
        cleanupEncoder();
        return false;
    }
    
    return true;
}

void StreamEncoder::cleanupEncoder()
{
    if (formatContext) {
        if (formatContext->pb) {
            av_write_trailer(formatContext);
            avio_closep(&formatContext->pb);
        }
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }
    
    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
    
    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }
    
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
    
    videoStream = nullptr;
}

void StreamEncoder::encodeFrame(const QImage &image)
{
    if (!streaming || !codecContext || !frame || !swsContext) {
        return;
    }
    
    // Convert QImage to RGB32 if needed
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB32);
    
    // Convert RGB to YUV
    const uint8_t *srcData[1] = { rgbImage.bits() };
    int srcLinesize[1] = { rgbImage.bytesPerLine() };
    
    sws_scale(swsContext, srcData, srcLinesize, 0, height,
              frame->data, frame->linesize);
    
    frame->pts = frameCount++;
    
    // Send frame to encoder
    int ret = avcodec_send_frame(codecContext, frame);
    if (ret < 0) {
        emit encodingError("Error sending frame to encoder");
        return;
    }
    
    // Receive encoded packets
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            emit encodingError("Error receiving packet from encoder");
            return;
        }
        
        // Rescale packet timestamps
        av_packet_rescale_ts(packet, codecContext->time_base, videoStream->time_base);
        packet->stream_index = videoStream->index;
        
        // Write packet to output
        ret = av_interleaved_write_frame(formatContext, packet);
        if (ret < 0) {
            emit encodingError("Error writing packet to output");
        }
        
        av_packet_unref(packet);
    }
}
