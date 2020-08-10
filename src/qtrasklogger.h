#pragma once

#include <QVector>
#include <QMutex>
#include <QFlags>
#include <QDateTime>
#include <QJsonObject>
#include "qtrasklogger_global.h"
#include "qtraskloggerwriter.h"

class QtRaskLoggerConfig;

class QTRASKLOGGER_EXPORT QtRaskLogger
{
public:
    enum class Level
    {
        Info = 0x1,
        Debug = 0x2,
        Error = 0x4,
        Warning = 0x8
    };

    Q_DECLARE_FLAGS(LogLevel, Level)

    static QtRaskLogger *getInstance(const QString &configFile = "");

    static void logger(QtMsgType type, const QMessageLogContext &logContext, const QString &msg);

    void enqueueMessage(const QString &message, Level type);

private:
    static QtRaskLogger *m_instance;
    QtRaskLogger::LogLevel m_level;
    bool m_showStd;
    QtRaskLoggerConfig *m_config;

    QtRaskLoggerWriter *m_loggerWriter;

    QtRaskLogger(const QString &configFile);
    ~QtRaskLogger() = default;
    QtRaskLogger(const QtRaskLogger &) = delete;
    QtRaskLogger& operator=(const QtRaskLogger &) = delete;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QtRaskLogger::LogLevel)

class QtRaskLoggerConfig
{
public:
    explicit QtRaskLoggerConfig(const QString &filename);

    QJsonObject config;

private:
    QString m_filename;

    void loadJsonFile();
};
