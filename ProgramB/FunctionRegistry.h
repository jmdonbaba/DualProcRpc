#pragma once

#include <QJsonObject>
#include <QString>
#include <functional>
#include <map>

using RpcFunction = std::function<QJsonObject(const QJsonObject &params)>;

class FunctionRegistry {
public:
    void registerFunction(const QString &name, RpcFunction func);
    bool hasFunction(const QString &name) const;
    QJsonObject invoke(const QString &name, const QJsonObject &params);
    QStringList functionNames() const;

private:
    std::map<QString, RpcFunction> m_functions;
};
