#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("AI Education App");
    app.setOrganizationName("AI Learning");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}