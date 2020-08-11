#pragma once

#include <QObject>
#include <QFile>
#include <QThread>
#include <QWaitCondition>
#include <QMutexLocker>
#include <QTimer>

class QtRaskLoggerWriter : public QThread
{
    Q_OBJECT

public:
    QtRaskLoggerWriter();

    ~QtRaskLoggerWriter();

    inline void setFilename(const QString &filename) { m_filename = filename; m_logFile.setFileName(filename); }

    inline void setMaxFileSize(const quint64 &size) { m_maxFileSize = size; }

    inline void setCompression(const bool &compression) { m_compression = compression; }

    inline void setRotateByDay(const bool &rotateByDay) { m_rotateByDay = rotateByDay; }

    void configure();

    void enqueue(const QString &message);

    inline bool writeInFile() { return m_logFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append); }

    inline void closeLogFile() { m_logFile.close(); }

    void write(const QString &message);

    void run() override;

signals:
    void startTimerRotateByDay();

private slots:
    void rotateByDay();

    void rotateBySize();

    bool isGzip(const QString &filename);

    void compress(const QString &filename);

private:
    QString m_filename;
    qint64 m_maxFileSize;
    bool m_compression;
    bool m_rotateByDay;
    QFile m_logFile;
    QVector<QString> m_messages;

    QWaitCondition m_queueNotEmpty;
    QMutex m_mutex;

    QTimer *m_timerRotateByDay;
    uint m_intervalRotateByDay;
};
