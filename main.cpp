#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // === 核心修复：解决 Ubuntu 下 WebEngine 与 QtCharts 混合导致 OpenGL 渲染崩溃/卡死/白屏的问题 ===
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
