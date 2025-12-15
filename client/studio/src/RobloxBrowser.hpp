

#pragma once

// Qt Headers
#include <QWebEngineView>

class RobloxBrowser : public QWebEngineView
{
    Q_OBJECT

public:
    RobloxBrowser(QWidget* parent = nullptr);
    virtual ~RobloxBrowser();

public Q_SLOTS:

    bool close();
    void resetLoadingTimer();

protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void dropEvent(QDropEvent* evt);

private Q_SLOTS:

    void loadStarted();
    void loadFinished(bool);

private:
    void drawLoadingWatermark();

    RobloxBrowser* m_pPopup;
    QDialog* m_pPopupDlg;
    QTimer* m_loadingTimer;
    float m_refreshIncr;
};