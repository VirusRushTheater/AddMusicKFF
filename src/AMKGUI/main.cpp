#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application properties
    app.setApplicationName("AMKGUI");
    app.setApplicationVersion("1.0.8");
    app.setOrganizationName("AddMusicK");

    // Set a native style
    app.setStyle(QStyleFactory::create("Fusion"));

    // Create and show main window
    MainWindow window;
    window.show();

    return app.exec();
}