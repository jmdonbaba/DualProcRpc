#pragma once

#include <QJsonObject>
#include <QDateTime>

struct BusinessFunctions {
    static QJsonObject add(const QJsonObject &params);
    static QJsonObject subtract(const QJsonObject &params);
    static QJsonObject multiply(const QJsonObject &params);
    static QJsonObject divide(const QJsonObject &params);
    static QJsonObject echo(const QJsonObject &params);
    static QJsonObject getTime(const QJsonObject &params);
    static QJsonObject getVersion(const QJsonObject &params);
    static QJsonObject arraySum(const QJsonObject &params);
    static QJsonObject toUpperCase(const QJsonObject &params);
};
