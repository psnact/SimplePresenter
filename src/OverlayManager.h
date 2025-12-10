#ifndef OVERLAYMANAGER_H
#define OVERLAYMANAGER_H

#include <QObject>
#include <QRect>
#include <QFont>
#include <QColor>
#include <QString>

class OverlayManager : public QObject
{
    Q_OBJECT

public:
    explicit OverlayManager(QObject *parent = nullptr);
    ~OverlayManager();
    
    // Overlay positioning
    void setPosition(const QRect &rect);
    QRect getPosition() const { return position; }
    
    // Overlay styling
    void setFont(const QFont &font);
    QFont getFont() const { return textFont; }
    
    void setTextColor(const QColor &color);
    QColor getTextColor() const { return textColor; }
    
    void setBackgroundColor(const QColor &color);
    QColor getBackgroundColor() const { return backgroundColor; }
    
    // Alignment presets
    void alignTop();
    void alignCenter();
    void alignBottom();
    void alignLeft();
    void alignRight();
    
    // Reset to defaults
    void resetToDefaults();

signals:
    void overlayChanged();

private:
    QRect position;
    QFont textFont;
    QColor textColor;
    QColor backgroundColor;
};

#endif // OVERLAYMANAGER_H
