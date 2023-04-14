#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>

#include <iostream>

#include <filesmodel.h>

namespace Application {
    Q_NAMESPACE
}

int main(int argc, char *argv[])
{
    qmlRegisterType<FilesModel>("Application", 1, 0, "FilesModel");
    //FilesModel g_filesModel;
    //g_filesModel.update("d:\\images\\");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;


    //qmlRegisterType("Application", 1, 0, "FilesModel", &filesModel);
    return app.exec();
}
