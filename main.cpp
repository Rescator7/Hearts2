#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFont>

int main(int argc, char *argv[])
{
    qputenv("QT_MEDIA_BACKEND", "ffmpeg");

    QApplication app(argc, argv);

    QFont font("Noto Sans", 11);
    font.setStyleName("Regular");

    MainWindow w;
    w.show();
    return app.exec();
}
