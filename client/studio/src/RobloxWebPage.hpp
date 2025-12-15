

#pragma once

#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QNetworkReply>
#include <QPoint>

class RobloxWebPage : public QWebEnginePage
{
    Q_OBJECT

    QStringList chooseFiles(FileSelectionMode mode, const QStringList& oldFiles, const QStringList& acceptedMimeTypes) override
    {
        if (!m_overideUploadFile.isEmpty())
        {
            return QStringList(m_overideUploadFile);
        }
        else
        {
            return QWebEnginePage::chooseFiles(mode, oldFiles, acceptedMimeTypes);
        }
    }

public:
    explicit RobloxWebPage(QWidget* parent = nullptr);

    QString getDefaultUserAgent() const; // To get access to protected default user agent
    void setUploadFile(QString selector, QString fileName);

    virtual void triggerAction(QWebEnginePage::WebAction action, bool checked = false);

protected:
    bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) override;
    bool event(QEvent* evt) override;

private Q_SLOTS:

    void handleFinished(QNetworkReply*);

private:
    QString m_overideUploadFile;
    QPoint m_contextPos;
};
