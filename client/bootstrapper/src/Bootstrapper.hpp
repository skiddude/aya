#pragma once

#include <string>
#include <rapidjson/document.h>
#include <QDialog>
#include <QLabel>
#include <QProgressDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class Bootstrapper
{
private:
    std::string mode;
    bool showUI;
    bool forceSkipUpdates;
    bool isUsingInstance;
    std::string instanceUrl;
    std::string instanceAccessKey;

    std::string httpGet(const std::string& url);
    int downloadFile(const std::string& url, const std::string& outputPath);

    rapidjson::Document fetchCachedManifest();
    rapidjson::Document fetchLatestManifest();
    void updateCachedManifest(const rapidjson::Document& manifest);

    static void extractTarZst(const std::string& archivePath, const std::string& outputDir);
    static bool verifySHA256(const std::string& filePath, const std::string& expectedHex);
    static rapidjson::Document parseJson(const std::string& jsonStr);

public:
    Bootstrapper(const std::string& mode, bool showUI, bool forceSkipUpdates, bool isUsingInstance, const std::string& instanceUrl,
        const std::string& instanceAccessKey);

    void update(const rapidjson::Document& manifest);
    void checkForUpdates();
    void start(const std::string& commandLine);
};

/*
class BootstrapperProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BootstrapperProgressDialog();

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onFinished();
    void onReadyRead();
    void onError(QNetworkReply::NetworkError code);

private:
    QString formatSize(qint64 bytes) const;

    QProgressDialog* progressDialog{nullptr};
    QNetworkAccessManager* manager{nullptr};
    QNetworkReply* reply{nullptr};
};
*/