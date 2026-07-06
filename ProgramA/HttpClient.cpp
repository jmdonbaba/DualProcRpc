#include "HttpClient.h"
#include "shared/Protocol.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &HttpClient::onReplyFinished);
}

void HttpClient::setBaseUrl(const QString &host, quint16 port)
{
    m_baseUrl.setScheme("http");
    m_baseUrl.setHost(host);
    m_baseUrl.setPort(port);
    m_baseUrl.setPath("/rpc");
}

void HttpClient::sendRequest(const RpcRequest &request)
{
    QNetworkRequest req(m_baseUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray body =
        QJsonDocument(request.toJson()).toJson(QJsonDocument::Compact);

    emit logMessage(QString("[HTTP] Sending request: %1").arg(request.function));
    m_nam->post(req, body);
}

void HttpClient::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    int statusCode =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    QByteArray body = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(
            QString("Network error: %1 (HTTP %2)")
                .arg(reply->errorString())
                .arg(statusCode));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit errorOccurred(
            QString("JSON parse error: %1").arg(parseError.errorString()));
        return;
    }

    RpcResponse resp = RpcResponse::fromJson(doc.object());
    emit logMessage(QString("[HTTP] Response: %1").arg(resp.status));
    emit responseReceived(resp);
}
