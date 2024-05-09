#include "FileMonitoringManager.h"
#include "JsonDtoFormat.h"

#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>

FileMonitoringManager::FileMonitoringManager(FileStorageManager *fsm,
                                             FileSystemEventStore *fses,
                                             QObject *parent)
    : QObject{parent}
{
    this->fsm = fsm;
    this->fses = fses;

    QObject::connect(&fileSystemEventListener, &FileSystemEventListener::signalAddEventDetected,
                     this, &FileMonitoringManager::slotOnAddEventDetected);

    QObject::connect(&fileSystemEventListener, &FileSystemEventListener::signalDeleteEventDetected,
                     this, &FileMonitoringManager::slotOnDeleteEventDetected);

    QObject::connect(&fileSystemEventListener, &FileSystemEventListener::signalModificationEventDetected,
                     this, &FileMonitoringManager::slotOnModificationEventDetected);

    QObject::connect(&fileSystemEventListener, &FileSystemEventListener::signalMoveEventDetected,
                     this, &FileMonitoringManager::slotOnMoveEventDetected);

    fileWatcher.watch();
}

FileMonitoringManager::~FileMonitoringManager()
{
    QHashIterator<QString, efsw::WatchID> hashIter(watchedFolderMap);

    hashIter.next();

    while(hashIter.hasNext())
    {
        fileWatcher.removeWatch(hashIter.value());
        hashIter.next();
    }

    delete fsm;
    delete fses;
    fsm = nullptr;
    fses = nullptr;
}

void FileMonitoringManager::addFolder(const QString &userFolderPath)
{
    efsw::WatchID watchId = fileWatcher.addWatch(userFolderPath.toStdString(), &fileSystemEventListener, true);

    if(watchId >= efsw::Errors::NoError)
        watchedFolderMap.insert(userFolderPath, watchId);
}

void FileMonitoringManager::slotOnAddEventDetected(const QString &fileOrFolderName, const QString &dir)
{
    //fses->addFolder(dir + fileOrFolderName, FileSystemEventStore::Status::NewAdded);

    QString completePath = dir + fileOrFolderName;

    handleFileModificationEvent(completePath);
}

void FileMonitoringManager::slotOnDeleteEventDetected(const QString &fileOrFolderName, const QString &dir)
{
    QString completePath = dir + fileOrFolderName;

    handleFolderDeleteEvent(completePath);
    handleFileDeleteEvent(completePath);
}

void FileMonitoringManager::slotOnModificationEventDetected(const QString &fileName, const QString &dir)
{
    QString completePath = dir + fileName;

    handleFileModificationEvent(completePath);
}

void FileMonitoringManager::slotOnMoveEventDetected(const QString &fileOrFolderName, const QString &oldFileOrFolderName, const QString &dir)
{
    //fses->addFolder(dir + fileOrFolderName, FileSystemEventStore::Status::Renamed);
}

void FileMonitoringManager::handleFolderDeleteEvent(const QString &userFolderPath)
{
    QString _userFolderPath = QDir::toNativeSeparators(userFolderPath);

    if(!_userFolderPath.endsWith(QDir::separator()))
        _userFolderPath.append(QDir::separator());

    QJsonObject folderJson = fsm->getFolderJsonByUserPath(_userFolderPath);
    bool isFolderPersists = folderJson[JsonKeys::IsExist].toBool();
    bool isFolderFrozen = folderJson[JsonKeys::Folder::IsFrozen].toBool();

    if(isFolderPersists && !isFolderFrozen) // TODO: Add check here for whether file is ignored for events.
        fses->addFolder(_userFolderPath, FileSystemEventStore::Status::Deleted);
}

void FileMonitoringManager::handleFileModificationEvent(const QString &userFilePath)
{
    QJsonObject fileJson = fsm->getFileJsonByUserPath(userFilePath);
    bool isFilePersists = fileJson[JsonKeys::IsExist].toBool();
    bool isFileFrozen = fileJson[JsonKeys::File::IsFrozen].toBool();

    if(isFilePersists && !isFileFrozen) // TODO: Add check here for whether file is ignored for events.
    {
        QString symbolPath = fileJson[JsonKeys::File::SymbolFilePath].toString();
        qlonglong versionNumber = fileJson[JsonKeys::File::MaxVersionNumber].toInteger();

        QJsonObject versionJson = fsm->getFileVersionJson(symbolPath, versionNumber);

        QString hash = versionJson[JsonKeys::FileVersion::Hash].toString();

        QFile file(userFilePath);
        bool isOpen = file.open(QFile::OpenModeFlag::ReadOnly);

        if(!isOpen) // TODO: Add more preliminary checks like file.exist(), isFile() etc...
            return;

        QCryptographicHash hasher(QCryptographicHash::Algorithm::Sha3_256);
        hasher.addData(&file);
        QString fileHash = QString(hasher.result().toHex());

        bool isHashChanged = (hash != fileHash);

        if(isHashChanged)
            fses->addFile(userFilePath, FileSystemEventStore::Status::Updated);
    }
}

void FileMonitoringManager::handleFileDeleteEvent(const QString &userFilePath)
{
    QJsonObject fileJson = fsm->getFileJsonByUserPath(userFilePath);
    bool isFilePersists = fileJson[JsonKeys::IsExist].toBool();
    bool isFileFrozen = fileJson[JsonKeys::File::IsFrozen].toBool();

    if(isFilePersists && !isFileFrozen) // TODO: Add check here for whether file is ignored for events.
        fses->addFile(userFilePath, FileSystemEventStore::Status::Deleted);
}
