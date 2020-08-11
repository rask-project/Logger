#include "qtrasklogger.h"

#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonArray>

#include <QDebug>

QtRaskLogger *QtRaskLogger::m_instance = nullptr;

QtRaskLogger *QtRaskLogger::getInstance(const QString &configFile)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    if (m_instance == nullptr) {
        m_instance = new QtRaskLogger(configFile);
        return m_instance;
    }
    return m_instance;
}

void QtRaskLogger::enqueueMessage(const QString &message, QtRaskLogger::Level type)
{
    if (m_level & type)
        m_loggerWriter->enqueue(message);
    if (m_showStd)
        qDebug().noquote() << message;
}

QtRaskLogger::QtRaskLogger(const QString &configFile):
    m_level(),
    m_config(new QtRaskLoggerConfig(configFile)),
    m_loggerWriter(nullptr)
{
    for (const auto &level: m_config->config["level"].toArray()) {
        if (level.toString() == "Debug")
            m_level |= QtRaskLogger::Level::Debug;
        if (level.toString() == "Info")
            m_level |= QtRaskLogger::Level::Info;
        if (level.toString() == "Error")
            m_level |= QtRaskLogger::Level::Error;
        if (level.toString() == "Warning")
            m_level |= QtRaskLogger::Level::Warning;
    }

    m_loggerWriter = new QtRaskLoggerWriter;
    m_loggerWriter->setFilename(m_config->config["filename"].toString());
    m_loggerWriter->setMaxFileSize(m_config->config["maxFileSize"].toInt());
    m_loggerWriter->setCompression(m_config->config["compression"].toBool());
    m_loggerWriter->setRotateByDay(m_config->config["rotateByDay"].toBool());
    m_loggerWriter->configure();
    m_loggerWriter->start();

    if (!m_level)
        m_level = QtRaskLogger::Level::Info | QtRaskLogger::Level::Debug | QtRaskLogger::Level::Error | QtRaskLogger::Level::Warning;
    m_showStd = m_config->config["showStd"].toBool();
}

void QtRaskLogger::logger(QtMsgType type, const QMessageLogContext &logContext, const QString &msg)
{
    QString typeStr;
    QtRaskLogger::Level level;

    switch (type) {
    case QtDebugMsg:
        typeStr = "Debug";
        level = QtRaskLogger::Level::Debug;
        break;
    case QtWarningMsg:
        typeStr = "Warning";
        level = QtRaskLogger::Level::Warning;
        break;
    case QtCriticalMsg:
        typeStr = "Error";
        level = QtRaskLogger::Level::Error;
        break;
    case QtInfoMsg:
        typeStr = "Info";
        level = QtRaskLogger::Level::Info;
        break;
    case QtFatalMsg:
        typeStr = "Fatal";
        level = QtRaskLogger::Level::Error;
        break;
    default:
        typeStr = "Debug";
        level = QtRaskLogger::Level::Debug;
    }

    QString message;
    if (logContext.file && logContext.function)
        message = QString("%1 [%2] [%3: %4 - %5] %6")
                .arg(QDateTime::currentDateTime().toString(Qt::DateFormat::ISODate))
                .arg(typeStr)
                .arg(logContext.file)
                .arg(logContext.line)
                .arg(logContext.function)
                .arg(msg);
    else
        message = QString("%1 [%2] %3").arg(QDateTime::currentDateTime().toString(Qt::DateFormat::ISODate)).arg(typeStr).arg(msg);

    QtRaskLogger::getInstance()->enqueueMessage(message, level);
}

QtRaskLoggerConfig::QtRaskLoggerConfig(const QString &filename):
    m_filename(filename)
{
    config = QJsonObject({
                             { "filename", "/tmp/qtrasklogger.log" },
                             { "maxFileSize", 2 },
                             { "compression", true },
                             { "rotateByDay", true },
                             { "level", QJsonArray({
                                   "Debug", "Info", "Error", "Warn"
                               }) },
                             { "showStd", true }
                         });

    if (!m_filename.isEmpty())
        loadJsonFile();
}

void QtRaskLoggerConfig::loadJsonFile()
{
    QFile file(m_filename);
    if (!file.exists()) {
        qWarning() << "Config file" << m_filename << "not found";
        qCritical() << "Using default configuration" << config;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Config file" << m_filename << "was not loaded";
        qCritical() << "Using default configuration" << config;
        return;
    }

    QTextStream stream(&file);
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(stream.readAll().toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCritical() << "An error occurred while parsing JSON file:" << error.errorString();
        qCritical() << "Using default configuration" << config;
        return;
    }

    if (!doc.isObject()) {
        qCritical() << "JSON content is not a valid object";
        return;
    }

    for (const auto &key: doc.object().keys()) {
        if  (config[key].type() == QJsonValue::Bool)
            config[key] = doc.object()[key].toBool();
        if  (config[key].type() == QJsonValue::Double)
            config[key] = doc.object()[key].toDouble();
        if  (config[key].type() == QJsonValue::String)
            config[key] = doc.object()[key].toString();
        if  (config[key].type() == QJsonValue::Array)
            config[key] = doc.object()[key].toArray();
        if  (config[key].type() == QJsonValue::Object)
            config[key] = doc.object()[key].toObject();
        if  (config[key].type() == QJsonValue::Null)
            config[key] = 0;
    }

    file.close();
}
