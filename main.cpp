#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFont>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QDebug>

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QFile logFile(QDir::homePath() + "/.hearts.log");

    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream out(&logFile);
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    switch (type) {
    case QtDebugMsg:
        out << timestamp << " DEBUG: " << msg << "\n";
        break;
    case QtInfoMsg:
        out << timestamp << " INFO: " << msg << "\n";
        break;
    case QtWarningMsg:
        out << timestamp << " WARNING: " << msg << "\n";
        break;
    case QtCriticalMsg:
        out << timestamp << " CRITICAL: " << msg << "\n";
        break;
    case QtFatalMsg:
        out << timestamp << " FATAL: " << msg << "\n";
        abort();
    }

    out.flush();
}

int main(int argc, char *argv[])
{
    qputenv("QT_MEDIA_BACKEND", "ffmpeg");
    QFile::remove(QDir::homePath() + "/.hearts.log");

    qInstallMessageHandler(messageHandler);

    QApplication app(argc, argv);

    qDebug() << "=== Démarrage de Hearts" << QCoreApplication::applicationVersion()
             << "le" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "===";

    QFont font("Noto Sans", 11);
    font.setStyleName("Regular");

    MainWindow w;
    w.show();
    return app.exec();
}
