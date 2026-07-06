#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

struct RpcRequest;
struct RpcResponse;

class HttpClient : public QObject {
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &host, quint16 port);
    void sendRequest(const RpcRequest &request);

signals:
    void responseReceived(const RpcResponse &response);
    void errorOccurred(const QString &error);
    void logMessage(const QString &msg);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_nam = nullptr;
    QUrl m_baseUrl;
};
