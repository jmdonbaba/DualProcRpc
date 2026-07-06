#include "WebSocketClient.h"
#include "shared/Protocol.h"

#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_clientId(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(3000);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &WebSocketClient::onReconnectTimer);
}

void WebSocketClient::connectToServer(const QString &host, quint16 port)
{
    m_intentionalDisconnect = false;
    m_url.setScheme("ws");
    m_url.setHost(host);
    m_url.setPort(port);
    m_url.setPath("/");

    if (m_socket) {
        m_socket->deleteLater();
    }

    m_socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(m_socket, &QWebSocket::connected,
            this, &WebSocketClient::onConnected);
    connect(m_socket, &QWebSocket::textMessageReceived,
            this, &WebSocketClient::onTextMessage);
    connect(m_socket, &QWebSocket::disconnected,
            this, &WebSocketClient::onDisconnected);
    connect(m_socket, &QWebSocket::errorOccurred,
            this, &WebSocketClient::onError);

    emit logMessage(QString("[WS] Connecting to %1:%2...").arg(host).arg(port));
    m_socket->open(m_url);
}

void WebSocketClient::disconnectFromServer()
{
    m_intentionalDisconnect = true;
    m_reconnectTimer->stop();
    if (m_socket) {
        m_socket->close();
    }
}

bool WebSocketClient::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

QString WebSocketClient::clientId() const { return m_clientId; }

void WebSocketClient::onConnected()
{
    m_reconnectTimer->stop();
    emit logMessage("[WS] Connected, registering client...");

    // Send registration message
    QJsonObject reg;
    reg["type"]     = "register";
    reg["clientId"] = m_clientId;

    m_socket->sendTextMessage(
        QString::fromUtf8(QJsonDocument(reg).toJson(QJsonDocument::Compact)));
    emit connected();
}

void WebSocketClient::onTextMessage(const QString &message)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit logMessage(QString("[WS] JSON parse error: %1")
                            .arg(parseError.errorString()));
        return;
    }

    QJsonObject json = doc.object();
    QString type = json["type"].toString();

    if (type == "registered") {
        emit logMessage(
            QString("[WS] Registered as %1").arg(json["clientId"].toString()));
    } else {
        // Treat as RPC response
        RpcResponse resp = RpcResponse::fromJson(json);
        emit logMessage(QString("[WS] Async response: %1").arg(resp.status));
        emit responseReceived(resp);
    }
}

void WebSocketClient::onDisconnected()
{
    emit logMessage("[WS] Disconnected");
    emit disconnected();

    if (!m_intentionalDisconnect) {
        emit logMessage("[WS] Will reconnect in 3s...");
        m_reconnectTimer->start();
    }
}

void WebSocketClient::onError()
{
    QString errMsg = m_socket ? m_socket->errorString() : "Unknown error";
    emit logMessage(QString("[WS] Error: %1").arg(errMsg));
    emit errorOccurred(errMsg);
}

void WebSocketClient::onReconnectTimer()
{
    if (!m_intentionalDisconnect && m_socket && !m_url.isEmpty()) {
        emit logMessage("[WS] Reconnecting...");
        m_socket->open(m_url);
    }
}
