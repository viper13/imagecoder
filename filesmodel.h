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
        Compressed,
        NotCompressed,
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
        std::string fullName;
        QString suffix;
        qint64 size;
        FileStatus::Status status;
    };



    enum Params
    {
        Name = 0,
        Size = 1,
        Status = 2,

        Last = 3
    };

    Q_PROPERTY(QString path READ getPath WRITE setPath NOTIFY pathChanged)

public:
    explicit FilesModel(QObject *parent = nullptr);

    virtual ~FilesModel() override = default;

    QVariant data(const QModelIndex &index, int role) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QHash<int,QByteArray> roleNames() const override;

    void setPath(const QString& value);

    const QString& getPath() const { return path; }

    Q_INVOKABLE void compressFile(const int idx);

    Q_INVOKABLE void decompressFile(const int idx);

    Q_INVOKABLE void update();

signals:
    void pathChanged();
    void errorHappens(const QString message) const;

private:
    void finishPrecessing(const std::string &fileName);
    FileStatus::Status getFileStatusBySuffix(const QString& suffix) const;

    std::vector<FileData> modelData;
    QString path;
};

#endif // FILESMODEL_H
