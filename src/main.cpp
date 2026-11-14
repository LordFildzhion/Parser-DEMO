#include <QApplication>
#include <QCoreApplication>

#include "gui/mainwindow.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("MTSV"));
    QCoreApplication::setApplicationName(QStringLiteral("Analyzer"));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(ApplicationInfo::Version));

    MainWindow window;
    window.show();

    return application.exec();
}
