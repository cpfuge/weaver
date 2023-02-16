#include "main_window.hpp"

#include <QApplication>
#include <QStyleFactory>
#include "version.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    app.setApplicationName(WEAVER_VERSION_NAME);
    app.setApplicationVersion(WEAVER_VERSION_NUMBER);

#ifdef Q_OS_WIN
    const QString style = "Fusion";
    if (QStyleFactory::keys().contains(style))
    {
        app.setStyle(QStyleFactory::create(style));
    }
#endif // Q_OS_WIN

    MainWindow main_window;
    main_window.show();

    return app.exec();
}
