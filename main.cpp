#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>

#include <iostream>

#include <filesmodel.h>

int main(int argc, char *argv[])
{
    qmlRegisterType<FilesModel>("Application", 1, 0, "FilesModel");
    qmlRegisterUncreatableMetaObject(FileStatus::staticMetaObject, "Application", 1, 0, "FileStatus", "Error: only enum");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
