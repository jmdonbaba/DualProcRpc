#include "BusinessFunctions.h"
#include <QJsonArray>
#include <QDateTime>

QJsonObject BusinessFunctions::add(const QJsonObject &params)
{
    double a = params["a"].toDouble();
    double b = params["b"].toDouble();
    QJsonObject result;
    result["result"] = a + b;
    return result;
}

QJsonObject BusinessFunctions::subtract(const QJsonObject &params)
{
    double a = params["a"].toDouble();
    double b = params["b"].toDouble();
    QJsonObject result;
    result["result"] = a - b;
    return result;
}

QJsonObject BusinessFunctions::multiply(const QJsonObject &params)
{
    double a = params["a"].toDouble();
    double b = params["b"].toDouble();
    QJsonObject result;
    result["result"] = a * b;
    return result;
}

QJsonObject BusinessFunctions::divide(const QJsonObject &params)
{
    double a = params["a"].toDouble();
    double b = params["b"].toDouble();
    QJsonObject result;
    if (b == 0.0) {
        result["error"] = "Division by zero";
        return result;
    }
    result["result"] = a / b;
    return result;
}

QJsonObject BusinessFunctions::echo(const QJsonObject &params)
{
    QJsonObject result;
    result["echo"] = params;
    return result;
}

QJsonObject BusinessFunctions::getTime(const QJsonObject & /*params*/)
{
    QJsonObject result;
    result["time"]     = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    result["timestamp"] = QDateTime::currentSecsSinceEpoch();
    return result;
}

QJsonObject BusinessFunctions::getVersion(const QJsonObject & /*params*/)
{
    QJsonObject result;
    result["program"]  = "DualProcRpc-ProgramB";
    result["version"]  = "1.0.0";
    result["qtVersion"] = QT_VERSION_STR;
    return result;
}

QJsonObject BusinessFunctions::arraySum(const QJsonObject &params)
{
    QJsonArray arr = params["array"].toArray();
    double sum = 0;
    for (const auto &v : arr)
        sum += v.toDouble();
    QJsonObject result;
    result["result"] = sum;
    result["count"]  = arr.size();
    return result;
}

QJsonObject BusinessFunctions::toUpperCase(const QJsonObject &params)
{
    QString text = params["text"].toString();
    QJsonObject result;
    result["result"] = text.toUpper();
    return result;
}
