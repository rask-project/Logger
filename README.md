# QtRaskLogger
A simple and thread safe logger using Qt QMessageLogger

### Install
```bash
qmake Logger.pro
make
make install
```

### Config
The configuration is in a JSON file

| Parameter | Default | Description |
| --------- | ------- | ------- |
| filename | empty (string) | Target log file name |
| maxFileSize | 2 (int: integer in MB ) | Maximum file size to rotate |
| rotateByDay | true (boolean) | Enable/Disable rotate by day |
| compression | true (boolean) | Set whether compression is enabled |
| level | \["Error", "Info", "Warning", "Debug"\] (array) | Level of log |
| showStd | true (boolean) | Show in std output |

Example:
```json
{
  "filename": "/tmp/logs-teste/teste.log",
  "maxFileSize": 10,
  "compression": true,
  "level": ["Error", "Info", "Warning", "Debug"],
  "showStd": false
}
```

### Usage
Include in your \*.pro

```pro
LIBS += -lQtRaskLogger
```

```cpp
#include <QDebug>
#include <QtRaskLogger/QtRaskLogger>

int main(int argc, char *argv[])
{
    QtRaskLogger::getInstance("./config/log.json");
    qInstallMessageHandler(QtRaskLogger::logger);
    
    qDebug() << "Debug Example";
    qInfo() << "Info Example";
    qWarning() << "Warning / Error Example";
    qCritical() << "Critical / Error Example";
    
    return 0;
}
```
