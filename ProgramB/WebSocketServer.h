#pragma once

#include <QObject>
#include <QMap>
#include <QString>

class QWebSocketServer;
class QWebSocket;
class FunctionRegistry;
struct RpcResponse;

class WebSocketServer : public QObject {
    Q_OBJECT
public:
    explicit WebSocketServer(QObject *parent = nullptr);

    bool start(quint16 port);
    void stop();
    bool isRunning() const;
    quint16 port() const;

    void setFunctionRegistry(FunctionRegistry *registry);
    void pushToClient(const QString &clientId, const RpcResponse &response);

signals:
    void logMessage(const QString &msg);

private slots:
    void onNewConnection();
    void onTextMessage(const QString &message);
    void onDisconnected();

private:
    void handleRegister(QWebSocket *socket, const QString &clientId);
    void handleRpcCall(QWebSocket *socket, const QString &clientId,
                       const QJsonObject &json);

    QWebSocketServer *m_server = nullptr;
    FunctionRegistry *m_registry = nullptr;
    // Map from clientId to WebSocket connection
    QMap<QString, QWebSocket *> m_clientSockets;
    // Reverse map for disconnect tracking
    QMap<QWebSocket *, QString> m_socketClients;
    quint16 m_port = 0;
    bool m_running = false;
};
