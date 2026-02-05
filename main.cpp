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
  static const qint64 MAX_LOG_SIZE = 5 * 1024 * 1024;  // 5 Mo
  static bool loggingDisabled = false;

  if (msg.contains("invalid type") ||
    msg.contains("Trying to construct") ||
    msg.contains("QObject::disconnect") ||
    msg.contains("QGraphicsScene")) {
    return;
  }

  if (loggingDisabled) {
    return;
  }

  QFile logFile(QDir::homePath() + "/.hearts.log");

  if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    return;
  }

  qint64 currentSize = logFile.size();

  if (currentSize >= MAX_LOG_SIZE) {
    QTextStream out(&logFile);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
        << " [LOG LIMIT] Taille maximale atteinte (" << MAX_LOG_SIZE / 1024 / 1024 << " Mo). "
        << "Logging désactivé pour éviter une boucle infinie.\n";
    out.flush();
    logFile.close();

    loggingDisabled = true;
    qWarning() << "Log file size limit reached. Logging stopped.";
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

// for debugging
//    QByteArray localMsg = msg.toLocal8Bit();
//    fprintf(stderr, "%s\n", localMsg.constData());

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
