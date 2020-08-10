#include "qtraskloggerwriter.h"
#include "qtraskcompress.h"

#include <QDateTime>
#include <QTextStream>
#include <QDataStream>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDir>
#include <QDirIterator>
#include <QCollator>
#include <algorithm>
#include <QProcess>
#include <QDebug>

quint32 crc32buf(const QByteArray& data);

QtRaskLoggerWriter::QtRaskLoggerWriter() :
    m_filename(),
    m_maxFileSize(0),
    m_logFile(),
    m_messages(),
    m_queueNotEmpty(),
    m_mutex(),
    m_timerRotateByDay(new QTimer),
    m_intervalRotateByDay(10000)
{
    connect(this, &QtRaskLoggerWriter::filenameChanged, this, &QtRaskLoggerWriter::rotateByDay);
    connect(m_timerRotateByDay, &QTimer::timeout, this, &QtRaskLoggerWriter::rotateByDay);
    m_timerRotateByDay->start(m_intervalRotateByDay);
}

QtRaskLoggerWriter::~QtRaskLoggerWriter()
{
    delete m_timerRotateByDay;
}

void QtRaskLoggerWriter::enqueue(const QString &message)
{
    QMutexLocker locker(&m_mutex);
    m_messages.append(message);
}

void QtRaskLoggerWriter::write(const QString &message)
{
    QTextStream out(&m_logFile);
    out << message << '\n';
}

void QtRaskLoggerWriter::run()
{
    while (true) {
        if (m_messages.isEmpty())
            continue;

        decltype (m_messages) copy;
        {
            QMutexLocker locker(&m_mutex);
            std::swap(copy, m_messages);
        }

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
                compress(newName);
            }
            compress(filenameRotateByDay);
        }
    }

    QDateTime now = QDateTime::currentDateTime();
    m_intervalRotateByDay = midnigth.toTime_t() > now.toTime_t() ? now.msecsTo(midnigth) : midnigth.msecsTo(now);

    m_timerRotateByDay->setInterval(m_intervalRotateByDay);
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

    for (const auto &file: rotated)
        QFile(QString("%1/%2").arg(dir.path()).arg(file)).rename(QString("%1.%2").arg(m_filename).arg(rotate--));
    QFile(m_filename).rename(QString("%1.1").arg(m_filename));
}

void QtRaskLoggerWriter::compress(const QString &filename)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filename);
    if (mime.inherits("application/gzip"))
        return;

    QProcess processGzip;
    processGzip.start("gzip", QStringList() << "-9" << filename);
    processGzip.waitForFinished();
}
