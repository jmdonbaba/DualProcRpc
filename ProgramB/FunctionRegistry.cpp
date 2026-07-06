#include "FunctionRegistry.h"
#include <QJsonObject>

void FunctionRegistry::registerFunction(const QString &name, RpcFunction func)
{
    m_functions[name] = std::move(func);
}

bool FunctionRegistry::hasFunction(const QString &name) const
{
    return m_functions.find(name) != m_functions.end();
}

QJsonObject FunctionRegistry::invoke(const QString &name, const QJsonObject &params)
{
    auto it = m_functions.find(name);
    if (it == m_functions.end()) {
        QJsonObject err;
        err["error"] = QString("Function '%1' not found").arg(name);
        return err;
    }
    return it->second(params);
}

QStringList FunctionRegistry::functionNames() const
{
    QStringList names;
    for (const auto &pair : m_functions)
        names.append(pair.first);
    return names;
}
