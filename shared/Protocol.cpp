#include "Protocol.h"

QJsonObject RpcRequest::toJson() const
{
    QJsonObject obj;
    obj["function"]  = function;
    obj["params"]    = params;
    obj["requestId"] = requestId;
    obj["clientId"]  = clientId;
    obj["mode"]      = mode;
    return obj;
}

RpcRequest RpcRequest::fromJson(const QJsonObject &json)
{
    RpcRequest req;
    req.function  = json["function"].toString();
    req.params    = json["params"].toObject();
    req.requestId = json["requestId"].toString();
    req.clientId  = json["clientId"].toString();
    req.mode      = json["mode"].toString("sync");
    return req;
}

QJsonObject RpcResponse::toJson() const
{
    QJsonObject obj;
    obj["status"]    = status;
    obj["data"]      = data;
    obj["error"]     = error;
    obj["requestId"] = requestId;
    return obj;
}

RpcResponse RpcResponse::fromJson(const QJsonObject &json)
{
    RpcResponse resp;
    resp.status    = json["status"].toString();
    resp.data      = json["data"];
    resp.error     = json["error"].toString();
    resp.requestId = json["requestId"].toString();
    return resp;
}
