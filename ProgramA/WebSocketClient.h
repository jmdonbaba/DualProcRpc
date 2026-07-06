#pragma once

#include <QObject>
#include <QUrl>
#include <QTimer>

class QWebSocket;
struct RpcResponse;

class WebSocketClient : public QObject {
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;
    QString clientId() const;

signals:
    void connected();
    void disconnected();
    void responseReceived(const RpcResponse &response);
    void errorOccurred(const QString &error);
    void logMessage(const QString &msg);

private slots:
    void onConnected();
    void onTextMessage(const QString &message);
    void onDisconnected();
    void onError();
    void onReconnectTimer();

private:
    QWebSocket *m_socket = nullptr;
    QString m_clientId;
    QUrl m_url;
    QTimer *m_reconnectTimer = nullptr;
    bool m_intentionalDisconnect = false;
};
