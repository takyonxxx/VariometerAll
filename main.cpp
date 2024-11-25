#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyle(QStyleFactory::create("Fusion"));    // fusion look & feel of controls
    QPalette p (QColor(4, 50, 60));
    a.setPalette(p);

    MainWindow w;
    w.show();
    return a.exec();
}
