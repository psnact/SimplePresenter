#include "OverlayManager.h"

OverlayManager::OverlayManager(QObject *parent)
    : QObject(parent)
    , position(0, 0, 1920, 1080)
    , textColor(Qt::white)
    , backgroundColor(0, 0, 0, 180)
{
    textFont.setFamily("Arial");
    textFont.setPointSize(48);
    textFont.setBold(true);
}

OverlayManager::~OverlayManager()
{
}

void OverlayManager::setPosition(const QRect &rect)
{
    position = rect;
    emit overlayChanged();
}

void OverlayManager::setFont(const QFont &font)
{
    textFont = font;
    emit overlayChanged();
}

void OverlayManager::setTextColor(const QColor &color)
{
    textColor = color;
    emit overlayChanged();
}

void OverlayManager::setBackgroundColor(const QColor &color)
{
    backgroundColor = color;
    emit overlayChanged();
}

void OverlayManager::alignTop()
{
    position.moveTop(50);
    emit overlayChanged();
}

void OverlayManager::alignCenter()
{
    // Center vertically
    int screenHeight = 1080; // Default, should be configurable
    position.moveTop((screenHeight - position.height()) / 2);
    emit overlayChanged();
}

void OverlayManager::alignBottom()
{
    int screenHeight = 1080;
    position.moveTop(screenHeight - position.height() - 50);
    emit overlayChanged();
}

void OverlayManager::alignLeft()
{
    position.moveLeft(50);
    emit overlayChanged();
}

void OverlayManager::alignRight()
{
    int screenWidth = 1920;
    position.moveLeft(screenWidth - position.width() - 50);
    emit overlayChanged();
}

void OverlayManager::resetToDefaults()
{
    position = QRect(0, 0, 1920, 1080);
    textFont.setFamily("Arial");
    textFont.setPointSize(48);
    textFont.setBold(true);
    textColor = Qt::white;
    backgroundColor = QColor(0, 0, 0, 180);
    emit overlayChanged();
}
