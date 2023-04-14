#include "filesmodel.h"

#include <QDirIterator>
#include <QFile>

#include <iostream>

FilesModel::FilesModel(QObject *)
{

}

void FilesModel::update()
{
    beginResetModel();
    QDirIterator it(path, QStringList(), QDir::Files, QDirIterator::NoIteratorFlags);
    while (it.hasNext())
    {
        QFileInfo fileInfo(it.next());
        std::cout << fileInfo.fileName().toStdString() << " " << fileInfo.size() << "B" << std::endl;
        modelData.push_back({fileInfo.fileName(), "", fileInfo.size()});
    }
    endResetModel();
}

QVariant FilesModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= modelData.size()
            || role >= Params::Last)
    {
        return QVariant();
    }

    const auto& item = modelData.at(index.row());
    switch (role) {
        case Params::Name : return item.name;
        case Params::Extention : return item.ext;
        case Params::Size : return item.size;
    }
    return QVariant();
}

void FilesModel::setPath(const QString &value)
{
     if (value != path)
     {
         path = value;
         emit pathChanged();
         update();
     }
}

int FilesModel::rowCount(const QModelIndex &parent) const
{
    return static_cast<int>(modelData.size());
}

QHash<int, QByteArray> FilesModel::roleNames() const
{
    return {
        {static_cast<int>(Params::Name), "name"},
        {static_cast<int>(Params::Extention), "ext"},
        {static_cast<int>(Params::Size), "size"}
    };
}
