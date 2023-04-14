#ifndef FILESMODEL_H
#define FILESMODEL_H

#include <QAbstractListModel>
#include <QQmlEngine>
#include <vector>

namespace FileStatus
{
    Q_NAMESPACE
    enum Status
    {
        Unknown,
        Unsupported,
        Converted,
        Processing
    };
    Q_ENUM_NS(Status)
}

class FilesModel : public QAbstractListModel
{
    Q_OBJECT
    struct FileData
    {
        QString name;
        QString ext;
        qint64 size;
        FileStatus::Status status;
    };



    enum Params
    {
        Name = 0,
        Extention = 1,
        Size = 2,
        Status = 3,

        Last = 4
    };

    Q_PROPERTY(QString path READ getPath WRITE setPath NOTIFY pathChanged)

public:
    explicit FilesModel(QObject *parent = nullptr);

    virtual ~FilesModel() override = default;

    void update();

    QVariant data(const QModelIndex &index, int role) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QHash<int,QByteArray> roleNames() const override;

    void setPath(const QString& value);

    const QString& getPath() const { return path; }

signals:
    void pathChanged();

private:
    std::vector<FileData> modelData;
    QString path;
};

#endif // FILESMODEL_H
