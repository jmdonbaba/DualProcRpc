#include "WebSocketServer.h"
#include "FunctionRegistry.h"
#include "shared/Protocol.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>

WebSocketServer::WebSocketServer(QObject *parent)
    : QObject(parent)
{
}

bool WebSocketServer::start(quint16 port)
{
    if (m_running)
        return true;

    m_server = new QWebSocketServer("DualProcRpc-B",
                                     QWebSocketServer::NonSecureMode, this);

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit logMessage(QString("WebSocket server failed to start: %1")
                            .arg(m_server->errorString()));
        delete m_server;
        m_server = nullptr;
        return false;
    }

    connect(m_server, &QWebSocketServer::newConnection,
            this, &WebSocketServer::onNewConnection);

    m_port    = m_server->serverPort();
    m_running = true;
    emit logMessage(QString("WebSocket server started on port %1").arg(m_port));
    return true;
}

void WebSocketServer::stop()
{
    if (!m_running)
        return;

    m_server->close();

    // Disconnect all clients
    for (auto it = m_clientSockets.begin(); it != m_clientSockets.end(); ++it) {
        it.value()->close();
        it.value()->deleteLater();
    }
    m_clientSockets.clear();
    m_socketClients.clear();

    delete m_server;
    m_server  = nullptr;
    m_running = false;
    emit logMessage("WebSocket server stopped");
}

bool WebSocketServer::isRunning() const { return m_running; }
quint16 WebSocketServer::port() const { return m_port; }

void WebSocketServer::setFunctionRegistry(FunctionRegistry *registry)
{
    m_registry = registry;
}

void WebSocketServer::pushToClient(const QString &clientId,
                                    const RpcResponse &response)
{
    auto it = m_clientSockets.find(clientId);
    if (it == m_clientSockets.end()) {
        emit logMessage(QString("[WS] Client '%1' not connected, cannot push")
                            .arg(clientId));
        return;
    }

    QJsonDocument doc(response.toJson());
    it.value()->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    emit logMessage(QString("[WS] Pushed to client %1: %2")
                        .arg(clientId, response.status));
}

void WebSocketServer::onNewConnection()
{
    QWebSocket *socket = m_server->nextPendingConnection();
    if (!socket)
        return;

    connect(socket, &QWebSocket::textMessageReceived,
            this, &WebSocketServer::onTextMessage);
    connect(socket, &QWebSocket::disconnected,
            this, &WebSocketServer::onDisconnected);

    emit logMessage(QString("[WS] New connection from %1:%2")
                        .arg(socket->peerAddress().toString())
                        .arg(socket->peerPort()));
}

void WebSocketServer::onTextMessage(const QString &message)
{
    auto *socket = qobject_cast<QWebSocket *>(sender());
    if (!socket)
        return;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit logMessage(QString("[WS] JSON parse error from client: %1")
                            .arg(parseError.errorString()));
        return;
    }

    QJsonObject json = doc.object();
    QString type = json["type"].toString();

    if (type == "register") {
        QString clientId = json["clientId"].toString();
        if (!clientId.isEmpty())
            handleRegister(socket, clientId);
        else
            emit logMessage("[WS] Register message missing clientId");
    } else if (type == "rpc") {
        // Handle WebSocket-based RPC calls (alternative to HTTP)
        QString clientId = json["clientId"].toString();
        handleRpcCall(socket, clientId, json);
    } else {
        emit logMessage(QString("[WS] Unknown message type: %1").arg(type));
    }
}

void WebSocketServer::handleRegister(QWebSocket *socket, const QString &clientId)
{
    // Remove old registration if any
    if (m_socketClients.contains(socket)) {
        QString oldId = m_socketClients[socket];
        m_clientSockets.remove(oldId);
    }

    m_clientSockets[clientId]  = socket;
    m_socketClients[socket]    = clientId;

    emit logMessage(QString("[WS] Client registered: %1").arg(clientId));

    // Send confirmation
    QJsonObject ack;
    ack["type"]   = "registered";
    ack["clientId"] = clientId;
    socket->sendTextMessage(
        QString::fromUtf8(QJsonDocument(ack).toJson(QJsonDocument::Compact)));
}

void WebSocketServer::handleRpcCall(QWebSocket *socket, const QString &clientId,
                                     const QJsonObject &json)
{
    if (!m_registry) {
        RpcResponse resp;
        resp.status    = "error";
        resp.error     = "No function registry configured";
        resp.requestId = json["requestId"].toString();
        pushToClient(clientId, resp);
        return;
    }

    QString function = json["function"].toString();
    QJsonObject params = json["params"].toObject();
    QString requestId = json["requestId"].toString();

    emit logMessage(QString("[WS] RPC call from %1: %2").arg(clientId, function));

    if (!m_registry->hasFunction(function)) {
        RpcResponse resp;
        resp.status    = "error";
        resp.error     = QString("Function '%1' not found").arg(function);
        resp.requestId = requestId;
        pushToClient(clientId, resp);
        return;
    }

    QJsonObject resultData = m_registry->invoke(function, params);

    RpcResponse resp;
    resp.requestId = requestId;
    if (resultData.contains("error")) {
        resp.status = "error";
        resp.error  = resultData["error"].toString();
    } else {
        resp.status = "success";
        resp.data   = resultData;
    }

    pushToClient(clientId, resp);
}

void WebSocketServer::onDisconnected()
{
    auto *socket = qobject_cast<QWebSocket *>(sender());
    if (!socket)
        return;

    QString clientId = m_socketClients.take(socket);
    if (!clientId.isEmpty()) {
        m_clientSockets.remove(clientId);
        emit logMessage(QString("[WS] Client disconnected: %1").arg(clientId));
    }
    socket->deleteLater();
}
