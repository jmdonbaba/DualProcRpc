#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DualProcRpc-ProgramA");

    MainWindow window;
    window.show();

    return app.exec();
}
