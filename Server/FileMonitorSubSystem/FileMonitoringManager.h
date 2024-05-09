#ifndef FILEMONITORINGMANAGER_H
#define FILEMONITORINGMANAGER_H

#include <QObject>
#include <efsw/efsw.hpp>
#include "FileSystemEventStore.h"
#include "FileSystemEventListener.h"
#include "FileStorageSubSystem/FileStorageManager.h"

class FileMonitoringManager : public QObject
{
    Q_OBJECT
public:
    explicit FileMonitoringManager(FileStorageManager *fsm,
                                   FileSystemEventStore *fses,
                                   QObject *parent = nullptr);

    ~FileMonitoringManager();

    void addFolder(const QString &userFolderPath);

signals:

private slots:
    void slotOnAddEventDetected(const QString &fileOrFolderName, const QString &dir);
    void slotOnDeleteEventDetected(const QString &fileOrFolderName, const QString &dir);
    void slotOnModificationEventDetected(const QString &fileName, const QString &dir);
    void slotOnMoveEventDetected(const QString &fileOrFolderName, const QString &oldFileOrFolderName, const QString &dir);

private:
    void handleFolderDeleteEvent(const QString &userFolderPath);
    void handleFileModificationEvent(const QString &userFilePath);
    void handleFileDeleteEvent(const QString &userFilePath);


private:
    FileSystemEventListener fileSystemEventListener;
    efsw::FileWatcher fileWatcher;
    FileStorageManager *fsm;
    FileSystemEventStore *fses;
    QHash<QString, efsw::WatchID> watchedFolderMap;
};

#endif // FILEMONITORINGMANAGER_H
