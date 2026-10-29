#include "config.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setOrganizationName(QStringLiteral("ArdeshirV"));
    application.setOrganizationDomain(QStringLiteral("ardeshirv.github.io"));
    application.setApplicationName(QStringLiteral("AvestanEditor"));
    application.setApplicationVersion(Config::instance().appVersion());

    MainWindow window;
    window.show();
    return application.exec();
}
