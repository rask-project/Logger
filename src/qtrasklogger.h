#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QVector>
#include <QFlags>
#include <QDateTime>
#include <QJsonObject>
#include <memory>

#include "qtrasklogger_global.h"
#include "qtraskloggerwriter.h"

class QtRaskLoggerConfig;

class QTRASKLOGGER_EXPORT QtRaskLogger : public QObject
{
    Q_OBJECT
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

    std::unique_ptr<QtRaskLoggerWriter> m_loggerWriter;
    QThread m_threadLoggerWriter;

    QtRaskLogger(const QString &configFile, QObject *parent = nullptr);
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
