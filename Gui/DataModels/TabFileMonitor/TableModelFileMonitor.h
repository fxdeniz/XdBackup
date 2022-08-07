#ifndef TABLEMODELFILEMONITOR_H
#define TABLEMODELFILEMONITOR_H

#include <QSqlQueryModel>

class TableModelFileMonitor : public QSqlQueryModel
{
    Q_OBJECT
public:
    static const QString TABLE_NAME;
    static const QString COLUMN_NAME_NAME;
    static const QString COLUMN_NAME_PARENT_DIR;
    static const QString COLUMN_NAME_PATH;
    static const QString COLUMN_NAME_OLD_NAME;
    static const QString COLUMN_NAME_TYPE;
    static const QString COLUMN_NAME_STATUS;
    static const QString COLUMN_NAME_TIMESTAMP;
    static const QString COLUMN_NAME_ACTION;
    static const QString COLUMN_NAME_NOTE_NUMBER;

    enum ColumnIndex
    {
        Name = 0,
        ParentDir = 1,
        Path = 2,
        OldName = 3,
        Type = 4,
        Status = 5,
        Timestamp = 6,
        Action = 7,
        NoteNumber = 8
    };

    enum Action
    {
        Update,
        Replace,
        Delete,
        Freeze
    };

    enum ItemStatus
    {
        InvalidStatus,
        Missing,
        NewAdded,
        Modified,
        Moved,
        MovedAndModified,
        Deleted
    };

    enum ItemType
    {
        UndefinedType,
        Folder,
        File
    };

public:
    TableModelFileMonitor(QObject *parent = nullptr);

    static ItemStatus statusCodeFromString(const QString &status);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;

signals:

};

#endif // TABLEMODELFILEMONITOR_H
