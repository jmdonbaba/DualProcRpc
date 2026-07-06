#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QMap>
#include <QString>
#include <functional>

class FunctionRegistry;
class WebSocketServer;

class HttpServer : public QObject {
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = nullptr);

    bool start(quint16 port);
    void stop();
    bool isRunning() const;
    quint16 port() const;

    void setFunctionRegistry(FunctionRegistry *registry);
    void setWebSocketServer(WebSocketServer *wsServer);

signals:
    void logMessage(const QString &msg);
    void requestReceived(const QString &clientId, const QString &function,
                         const QString &params);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    struct ClientState {
        QByteArray buffer;
        enum { ReadingRequest, ReadingBody } state = ReadingRequest;
        QString method;
        QString path;
        QMap<QString, QString> headers;
        int contentLength = 0;
    };

    void processRequest(QTcpSocket *socket, ClientState &st);
    void sendResponse(QTcpSocket *socket, int statusCode,
                      const QString &statusText, const QByteArray &body);
    void sendErrorResponse(QTcpSocket *socket, int statusCode,
                           const QString &message);

    QTcpServer *m_server = nullptr;
    FunctionRegistry *m_registry = nullptr;
    WebSocketServer *m_wsServer = nullptr;
    QMap<QTcpSocket *, ClientState> m_clients;
    quint16 m_port = 0;
    bool m_running = false;
};
