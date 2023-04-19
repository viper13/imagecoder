#include "filesmodel.h"

#include "Coder/coder.h"

#include <QDirIterator>
#include <QFile>

#include <thread>
#include <iostream>

FilesModel::FilesModel(QObject *)
{

}

void FilesModel::update()
{
    beginResetModel();
    modelData.clear();
    QDirIterator it(path, QStringList(), QDir::Files, QDirIterator::NoIteratorFlags);
    while (it.hasNext())
    {
        QFileInfo fileInfo(it.next());
        FileStatus::Status status = getFileStatusBySuffix(fileInfo.suffix());
        modelData.push_back({fileInfo.fileName(), fileInfo.filePath().toStdString(), fileInfo.suffix(), fileInfo.size(), status});
    }
    endResetModel();
}

void FilesModel::finishPrecessing(const std::string &fileName)
{
    for (int it = 0; it < modelData.size(); ++it)
    {
        if (modelData[it].fullName == fileName)
        {
            modelData[it].status = getFileStatusBySuffix(modelData[it].suffix);
            emit dataChanged(index(it), index(it));
            break;
        }
    }
}

FileStatus::Status FilesModel::getFileStatusBySuffix(const QString &suffix) const
{
    FileStatus::Status status = FileStatus::Unknown;
    if (suffix == "bmp")
    {
        status = FileStatus::NotCompressed;
    }
    else if (suffix == "barch")
    {
        status = FileStatus::Compressed;
    }
    else
    {
        status = FileStatus::Unsupported;
    }
    return status;
}

QVariant FilesModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= modelData.size()
            || role >= Params::Last)
    {
        return QVariant();
    }

    const auto& item = modelData.at(index.row());
    switch (role)
    {
        case Params::Name : return item.name;
        case Params::Size : return item.size;
        case Params::Status : return item.status;
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

void FilesModel::compressFile(const int idx)
{
    if (idx >= modelData.size())
    {
        emit errorHappens("Strange: out of index!");
        return;
    }
    if (modelData[idx].status != FileStatus::NotCompressed)
    {
        if (modelData[idx].status == FileStatus::Unsupported)
        {
            emit errorHappens("File format is not supported!");
        }
        else
        {
            emit errorHappens("File must be not compressed!");
        }
        return;
    }

    modelData[idx].status = FileStatus::Processing;
    emit dataChanged(index(idx), index(idx));
    std::thread excutor([this, idx](){
        const auto fileName = modelData[idx].fullName;
        try
        {
            compress(fileName, fileName + ".barch");
        }
        catch (const std::exception& e)
        {
            std::cerr << "Compresing failed: " << e.what() << std::endl;
            emit errorHappens("Unsupported file format for commpresing, wrong bmp format or wrong file type!");
        }
        finishPrecessing(fileName);
    });
    excutor.detach();
}

void FilesModel::decompressFile(const int idx)
{
    if (idx >= modelData.size())
    {
        emit errorHappens("Strange: out of index!");
        return;
    }
    if (modelData[idx].status != FileStatus::Compressed)
    {
        if (modelData[idx].status == FileStatus::Unsupported)
        {
            emit errorHappens("File format is not supported!");
        }
        else
        {
            emit errorHappens("File must be compressed!");
        }
        return;
    }
    modelData[idx].status = FileStatus::Processing;
    emit dataChanged(index(idx), index(idx));
    std::thread excutor([this, idx](){
        const auto fileName = modelData[idx].fullName;
        try
        {
            decompress(fileName, fileName + ".bmp");
        }
        catch (const std::exception& e)
        {
            std::cerr << "Compresing failed: " << e.what() << std::endl;
            emit errorHappens("Unsupported file format for commpresing, wrong bmp format or wrong file type!");
        }
        finishPrecessing(fileName);
    });
    excutor.detach();
}

int FilesModel::rowCount(const QModelIndex &parent) const
{
    return static_cast<int>(modelData.size());
}

QHash<int, QByteArray> FilesModel::roleNames() const
{
    return {
        {static_cast<int>(Params::Name), "name"},
        {static_cast<int>(Params::Size), "size"},
        {static_cast<int>(Params::Status), "status"}
    };
}
