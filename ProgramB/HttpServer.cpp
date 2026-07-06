#include "HttpServer.h"
#include "FunctionRegistry.h"
#include "WebSocketServer.h"
#include "shared/Protocol.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDebug>

HttpServer::HttpServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection,
            this, &HttpServer::onNewConnection);
}

bool HttpServer::start(quint16 port)
{
    if (m_running)
        return true;

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit logMessage(QString("HTTP server failed to start: %1")
                            .arg(m_server->errorString()));
        return false;
    }

    m_port = m_server->serverPort();
    m_running = true;
    emit logMessage(QString("HTTP server started on port %1").arg(m_port));
    return true;
}

void HttpServer::stop()
{
    if (!m_running)
        return;

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        it.key()->disconnect();
        it.key()->deleteLater();
    }
    m_clients.clear();

    m_server->close();
    m_running = false;
    emit logMessage("HTTP server stopped");
}

bool HttpServer::isRunning() const { return m_running; }
quint16 HttpServer::port() const { return m_port; }

void HttpServer::setFunctionRegistry(FunctionRegistry *registry)
{
    m_registry = registry;
}

void HttpServer::setWebSocketServer(WebSocketServer *wsServer)
{
    m_wsServer = wsServer;
}

void HttpServer::onNewConnection()
{
    while (QTcpSocket *socket = m_server->nextPendingConnection()) {
        connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &HttpServer::onDisconnected);
        m_clients[socket] = ClientState{};
    }
}

void HttpServer::onReadyRead()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;

    auto it = m_clients.find(socket);
    if (it == m_clients.end())
        return;

    ClientState &st = it.value();
    st.buffer.append(socket->readAll());

    if (st.state == ClientState::ReadingRequest) {
        // Look for end of headers: \r\n\r\n
        int headerEnd = st.buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return;

        QByteArray headerBlock = st.buffer.left(headerEnd);
        QList<QByteArray> lines = headerBlock.split('\n');

        // Parse request line: METHOD /path HTTP/1.1
        if (lines.isEmpty()) {
            sendErrorResponse(socket, 400, "Bad Request");
            return;
        }
        QList<QByteArray> reqLine = lines[0].trimmed().split(' ');
        if (reqLine.size() < 2) {
            sendErrorResponse(socket, 400, "Bad Request");
            return;
        }
        st.method = QString::fromUtf8(reqLine[0]);
        st.path   = QString::fromUtf8(reqLine[1]);

        // Parse headers
        for (int i = 1; i < lines.size(); ++i) {
            QByteArray line = lines[i].trimmed();
            int colonPos = line.indexOf(':');
            if (colonPos > 0) {
                QString key = QString::fromUtf8(line.left(colonPos).trimmed()).toLower();
                QString val = QString::fromUtf8(line.mid(colonPos + 1).trimmed());
                st.headers[key] = val;
            }
        }

        st.contentLength = st.headers.value("content-length", "0").toInt();

        // Move past header block + \r\n\r\n
        st.buffer = st.buffer.mid(headerEnd + 4);
        st.state = ClientState::ReadingBody;
    }

    if (st.state == ClientState::ReadingBody) {
        if (st.buffer.size() < st.contentLength)
            return;

        processRequest(socket, st);

        // processRequest defers disconnectFromHost, so socket is still in map.
        // If it was removed (edge case), bail out to avoid use-after-free on |st|.
        if (!m_clients.contains(socket))
            return;
    }
}

void HttpServer::processRequest(QTcpSocket *socket, ClientState &st)
{
    QByteArray body = st.buffer.left(st.contentLength);

    emit logMessage(QString("[HTTP] %1 %2").arg(st.method, st.path));

    if (st.method != "POST") {
        sendErrorResponse(socket, 405, "Method Not Allowed");
        return;
    }

    if (!m_registry) {
        sendErrorResponse(socket, 500, "No function registry configured");
        return;
    }

    // Parse JSON body as RpcRequest
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        sendErrorResponse(socket, 400,
                          QString("JSON parse error: %1").arg(parseError.errorString()));
        return;
    }

    RpcRequest req = RpcRequest::fromJson(doc.object());

    emit requestReceived(req.clientId, req.function,
                         QJsonDocument(req.params).toJson(QJsonDocument::Compact));

    // Check if function exists
    if (!m_registry->hasFunction(req.function)) {
        RpcResponse resp;
        resp.status    = "error";
        resp.error     = QString("Function '%1' not found").arg(req.function);
        resp.requestId = req.requestId;

        QByteArray respBody =
            QJsonDocument(resp.toJson()).toJson(QJsonDocument::Compact);
        sendResponse(socket, 404, "Not Found", respBody);

        emit logMessage(QString("[HTTP] Error: function '%1' not found").arg(req.function));
        return;
    }

    // Invoke the function
    QJsonObject resultData = m_registry->invoke(req.function, req.params);

    // Build response
    RpcResponse resp;
    resp.requestId = req.requestId;

    if (resultData.contains("error")) {
        resp.status = "error";
        resp.error  = resultData["error"].toString();
    } else {
        resp.status = "success";
        resp.data   = resultData;
    }

    QByteArray respBody =
        QJsonDocument(resp.toJson()).toJson(QJsonDocument::Compact);

    // If async mode and WebSocket is available, also push via WebSocket
    if (req.mode == "async" && m_wsServer && !req.clientId.isEmpty()) {
        m_wsServer->pushToClient(req.clientId, resp);
        emit logMessage(QString("[WS] Pushed async result to client %1").arg(req.clientId));
    }

    sendResponse(socket, 200, "OK", respBody);
    emit logMessage(QString("[HTTP] Response sent for %1: %2")
                        .arg(req.function, resp.status));
}

void HttpServer::sendResponse(QTcpSocket *socket, int statusCode,
                               const QString &statusText, const QByteArray &body)
{
    QByteArray response;
    response += QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8();
    response += "Content-Type: application/json; charset=utf-8\r\n";
    response += QString("Content-Length: %1\r\n").arg(body.size()).toUtf8();
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;

    socket->write(response);
    socket->flush();
    // Defer disconnect so onDisconnected fires after current call stack unwinds,
    // avoiding use-after-free of ClientState references in onReadyRead/processRequest.
    QTimer::singleShot(0, socket, [socket]() {
        if (socket->state() == QAbstractSocket::ConnectedState)
            socket->disconnectFromHost();
    });
}

void HttpServer::sendErrorResponse(QTcpSocket *socket, int statusCode,
                                    const QString &message)
{
    QJsonObject err;
    err["status"]    = "error";
    err["error"]     = message;
    QByteArray body = QJsonDocument(err).toJson(QJsonDocument::Compact);
    sendResponse(socket, statusCode, message, body);
}

void HttpServer::onDisconnected()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket) {
        m_clients.remove(socket);
        socket->deleteLater();
    }
}
