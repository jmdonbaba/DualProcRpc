#pragma once

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>
#include <QUuid>

struct RpcRequest {
    QString function;
    QJsonObject params;
    QString requestId;
    QString clientId;
    QString mode; // "sync" or "async"

    QJsonObject toJson() const;
    static RpcRequest fromJson(const QJsonObject &json);
};

struct RpcResponse {
    QString status;   // "success" or "error"
    QJsonValue data;
    QString error;
    QString requestId;

    QJsonObject toJson() const;
    static RpcResponse fromJson(const QJsonObject &json);
};
