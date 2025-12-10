#ifndef VDONINJAWIDGET_H
#define VDONINJAWIDGET_H

#include <QWidget>

class QWebEngineView;

class VdoNinjaWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VdoNinjaWidget(QWidget *parent = nullptr);
    ~VdoNinjaWidget();

    void setUrl(const QString &url);
    void clear();
    QString currentUrl() const { return m_currentUrl; }

private:
    void setupWebView();

    QWebEngineView *m_webView;
    QString m_currentUrl;
};

#endif // VDONINJAWIDGET_H
