#include "qtraskloggerwriter.h"

#include <QDateTime>
#include <QTextStream>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDir>
#include <QDirIterator>
#include <QCollator>
#include <algorithm>
#include <QProcess>
#include <QDebug>

QtRaskLoggerWriter::QtRaskLoggerWriter(QObject *parent) :
    QObject(parent),
    m_filename(),
    m_maxFileSize(0),
    m_compression(false),
    m_rotateByDay(false),
    m_logFile(),
    m_messages(),
    m_mutex(),
    m_timerRotateByDay(nullptr),
    m_intervalRotateByDay()
{}

QtRaskLoggerWriter::~QtRaskLoggerWriter()
{}

void QtRaskLoggerWriter::configure()
{
    checkLogFileDir();
    if (m_rotateByDay) {
        rotateByDay();

        m_timerRotateByDay = std::make_unique<QTimer>();
        m_timerRotateByDay.get()->setInterval(m_intervalRotateByDay);
        connect(m_timerRotateByDay.get(), &QTimer::timeout, this, &QtRaskLoggerWriter::rotateByDay);
        m_timerRotateByDay.get()->start(m_intervalRotateByDay);
    }
}

void QtRaskLoggerWriter::enqueue(const QString &message)
{
    QMutexLocker locker(&m_mutex);
    m_messages.append(message);
}

void QtRaskLoggerWriter::checkLogFileDir()
{
    QFileInfo logFile(m_logFile);
    QDir logDir;

    if (!logDir.exists(logFile.absolutePath())) {
        if (!logDir.mkdir(logFile.absolutePath())) {
            qFatal("Create log file path");
        }
    }
}

void QtRaskLoggerWriter::write(const QString &message)
{
    QTextStream out(&m_logFile);
    out << message << '\n';
}

void QtRaskLoggerWriter::run()
{
    while (true) {
        QThread::msleep(5);
        decltype (m_messages) copy;
        {
            QMutexLocker locker(&m_mutex);
            std::swap(copy, m_messages);
        }

        if (copy.isEmpty())
            continue;

        if (!writeInFile())
            qFatal("Error: %s", m_logFile.errorString().toLatin1().constData());
        for (const auto &message : qAsConst(copy))
            write(message);
        closeLogFile();

        copy.clear();

        if (m_maxFileSize > 0 && m_logFile.size() >= m_maxFileSize * 1000000) // convert Byte to MB
            rotateBySize();
    }
}

void QtRaskLoggerWriter::rotateByDay()
{
    QFileInfo info(m_filename);
    QDateTime midnigth = QDateTime::fromString(QDate::currentDate().toString() + " " + QTime::fromString("23:59:59").toString());

    if (info.exists()) {
        if (info.lastModified().daysTo(QDateTime::currentDateTime())) {
            const QString filenameRotateByDay = QString("%1/%2-%3.log")
                    .arg(info.absolutePath())
                    .arg(info.lastModified().date().toString(Qt::DateFormat::ISODate))
                    .arg(info.baseName());

            QFile(info.filePath()).rename(filenameRotateByDay);

            QDirIterator dir(info.absolutePath(), QStringList() << QString("%1.*").arg(info.fileName()));
            while (dir.hasNext()) {
                QFileInfo file(dir.next());
                const QString newName = QString("%1/%2-%3")
                        .arg(file.absolutePath())
                        .arg(file.lastModified().date().toString(Qt::DateFormat::ISODate))
                        .arg(file.fileName());

                QFile(file.filePath()).rename(newName);
                if (!isGzip(file.fileName()) && m_compression)
                    compress(newName);
            }

            if (!isGzip(filenameRotateByDay) && m_compression)
                compress(filenameRotateByDay);
        }
    }

    QDateTime now = QDateTime::currentDateTime();
    m_intervalRotateByDay = midnigth.toTime_t() > now.toTime_t() ? now.msecsTo(midnigth) : midnigth.msecsTo(now);
}

void QtRaskLoggerWriter::rotateBySize()
{
    QFileInfo info(m_filename);
    QDir dir(info.absoluteDir());
    dir.setNameFilters(QStringList() << QString("%1.*").arg(info.fileName()));

    QStringList rotated = dir.entryList();
    quint16 rotate = rotated.length() + 1;
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(rotated.begin(), rotated.end(), [&collator](const QString &file1, const QString &file2)
    {
        return collator.compare(file1, file2) > 0;
    });

    for (const auto &file: rotated) {
        QString rotatedNewName;

        bool compressed = isGzip(file);
        if (compressed)
            rotatedNewName = QString("%1.%2.gz").arg(m_filename).arg(rotate--);
         else
            rotatedNewName = QString("%1.%2").arg(m_filename).arg(rotate--);

        QFile(QString("%1/%2").arg(dir.path()).arg(file)).rename(rotatedNewName);
        if (!compressed && m_compression)
            compress(rotatedNewName);
    }

    QString newName = QString("%1.1").arg(m_filename);
    QFile(m_filename).rename(newName);
    if (m_compression)
        compress(newName);
}

bool QtRaskLoggerWriter::isGzip(const QString &filename)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filename);
    if (mime.inherits("application/gzip"))
        return true;
    return false;
}

void QtRaskLoggerWriter::compress(const QString &filename)
{
    QProcess gzip;
    gzip.start("gzip", QStringList() << "-9" << filename);

    if (!gzip.waitForStarted())
        qFatal("%s", gzip.errorString().toLatin1().constData());

    if (!gzip.waitForFinished())
        qFatal("%s", gzip.errorString().toLatin1().constData());
}
