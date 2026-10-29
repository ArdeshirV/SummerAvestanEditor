#include "config.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    // Avoid Qt Widgets' QRhi/EGL backing-store path on Android. Some device/Qt
    // combinations abort when EGL surfaces are queried during IME callbacks.
    qputenv("QT_WIDGETS_RHI", "0");
#endif
    QApplication application(argc, argv);
    application.setOrganizationName(QStringLiteral("ArdeshirV"));
    application.setOrganizationDomain(QStringLiteral("ardeshirv.github.io"));
    application.setApplicationName(QStringLiteral("AvestanEditor"));
    application.setApplicationVersion(Config::instance().appVersion());

    MainWindow window;
    window.show();
    return application.exec();
}
