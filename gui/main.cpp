#include "main_window.hpp"

#include <QApplication>
#include <QIcon>
#include <QString>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setWindowIcon(
        QIcon(QStringLiteral(":/icons/tsp-app-icon.png")));
    tsp::desktop::MainWindow window;
    window.show();
    return application.exec();
}
