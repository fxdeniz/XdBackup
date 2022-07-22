#include "TabFileMonitor.h"
#include "ui_TabFileMonitor.h"
#include "Backend/FileStorageSubSystem/FileStorageManager.h"

#include <QtConcurrent>
#include <QFileInfo>
#include <QDir>

TabFileMonitor::TabFileMonitor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabFileMonitor)
{
    ui->setupUi(this);

    QList<TableModelFileMonitor::TableItem> sampleFileMonitorTableData;

    sampleFileMonitorTableData.insert(0, {"first_file.txt",
                                          "C:/Users/<USER>/AppData/Roaming/<APPNAME>/",
                                          "old location here",
                                          TableModelFileMonitor::TableItemType::File,
                                          TableModelFileMonitor::TableItemStatus::Deleted,
                                          QDateTime::currentDateTime()
                                         });

    sampleFileMonitorTableData.insert(1, {"second_file.zip",
                                          "C:/Users/<USER>/Desktop/",
                                          "old location here",
                                          TableModelFileMonitor::TableItemType::File,
                                          TableModelFileMonitor::TableItemStatus::Moved,
                                          QDateTime::fromString("2021-03-19 10:40:30", "yyyy-MM-dd HH:mm:ss")
                                         });

    sampleFileMonitorTableData.insert(2, {"third_file.pdf",
                                          "C:/Users/<USER>/Desktop/",
                                          "old location here",
                                          TableModelFileMonitor::TableItemType::File,
                                          TableModelFileMonitor::TableItemStatus::Modified,
                                          QDateTime::fromString("2019-12-27 03:50:00", "yyyy-MM-dd HH:mm:ss")
                                         });

    sampleFileMonitorTableData.insert(3, {"fourth_file.mp4",
                                          "C:/Users/<USER>/Videos/",
                                          "old location here",
                                          TableModelFileMonitor::TableItemType::File,
                                          TableModelFileMonitor::TableItemStatus::NewAdded,
                                          QDateTime::fromString("2022-01-15 07:00:00", "yyyy-MM-dd HH:mm:ss")
                                         });

    //this->tableModelFileMonitor = new TableModelFileMonitor(sampleFileMonitorTableData, this);
    this->tableModelFileMonitor = new TableModelFileMonitor(this);
    this->ui->tableViewFileMonitor->setModel(this->tableModelFileMonitor);
    this->ui->tableViewFileMonitor->horizontalHeader()->setMinimumSectionSize(110);
    this->ui->tableViewFileMonitor->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    this->ui->tableViewFileMonitor->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Interactive);
    this->ui->tableViewFileMonitor->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Interactive);
    this->ui->tableViewFileMonitor->resizeColumnsToContents();

    this->comboBoxItemDelegateNote = new ComboBoxItemDelegateNote(this->ui->tableViewFileMonitor);
    this->comboBoxItemDelegateFileAction = new ComboBoxItemDelegateFileAction(this->ui->tableViewFileMonitor);

    for(int rowIndex = 0; rowIndex < sampleFileMonitorTableData.size(); rowIndex++)
    {
        int columnActionIndex = 4;
        int columnNoteIndex = 5;

        this->ui->tableViewFileMonitor->setItemDelegateForColumn(columnNoteIndex, this->comboBoxItemDelegateNote);
        this->ui->tableViewFileMonitor->openPersistentEditor(this->tableModelFileMonitor->index(rowIndex, columnNoteIndex));


        this->ui->tableViewFileMonitor->setItemDelegateForColumn(columnActionIndex, this->comboBoxItemDelegateFileAction);
        this->ui->tableViewFileMonitor->openPersistentEditor(this->tableModelFileMonitor->index(rowIndex, columnActionIndex));
    }
}

TabFileMonitor::~TabFileMonitor()
{
    delete ui;
}

void TabFileMonitor::slotOnPredictionTargetNotFound(const QString &pathToFile)
{
    // TODO this will probably require another item
    QFileInfo fileInfo(pathToFile);
    auto fileDir = QDir::toNativeSeparators(fileInfo.absolutePath()) + QDir::separator();
    TableModelFileMonitor::TableItem item {
        fileInfo.fileName(),
        fileDir,
        "",
        TableModelFileMonitor::TableItemType::File,
        TableModelFileMonitor::TableItemStatus::Missing,
        QDateTime::currentDateTime()
    };

    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotOnUnPredictedFolderDetected(const QString &pathToFolder)
{
    auto item = TableModelFileMonitor::tableItemNewAddedFolderFrom(pathToFolder);
    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotOnNewFolderAdded(const QString &pathToFolder)
{
    auto *watcher = new QFutureWatcher<TableModelFileMonitor::TableItem>(this);
    resultSet.insert(watcher);
    QObject::connect(watcher, &QFutureWatcher<TableModelFileMonitor::TableItem>::finished,
                     this, &TabFileMonitor::slotRefreshTableViewFileMonitor);

    auto future = QtConcurrent::run([=]{
                      auto fsm = FileStorageManager::instance();
                      bool isFolderExistInDb = fsm->isFolderExistByUserFolderPath(pathToFolder);
                      return isFolderExistInDb;

                  }).then(QtFuture::Launch::Inherit, [=](QFuture<bool> previous){
                          TableModelFileMonitor::TableItem item;

                          if(previous.result() == true)
                              item = TableModelFileMonitor::tableItemModifiedFolderFrom(pathToFolder);
                          else
                              item = TableModelFileMonitor::tableItemNewAddedFolderFrom(pathToFolder);

                          return item;
                      });

    watcher->setFuture(future);

//    auto item = TableModelFileMonitor::tableItemNewAddedFolderFrom(pathToFolder);
//    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotOnFolderDeleted(const QString &pathToFolder)
{
    auto item = TableModelFileMonitor::tableItemDeletedFolderFrom(pathToFolder);
    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotOnFolderMoved(const QString &pathToFolder, const QString &oldFolderName)
{
    auto item = TableModelFileMonitor::tableItemMovedFolderFrom(pathToFolder, oldFolderName);
    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotOnUnPredictedFileDetected(const QString &pathToFile)
{
    auto item = TableModelFileMonitor::tableItemNewAddedFileFrom(pathToFile);
    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotOnNewFileAdded(const QString &pathToFile)
{
    auto *watcher = new QFutureWatcher<TableModelFileMonitor::TableItem>(this);
    resultSet.insert(watcher);
    QObject::connect(watcher, &QFutureWatcher<TableModelFileMonitor::TableItem>::finished,
                     this, &TabFileMonitor::slotRefreshTableViewFileMonitor);

    auto future = QtConcurrent::run([=]{

        TableModelFileMonitor::TableItem item;
        auto fsm = FileStorageManager::instance();

        bool isFileExistInDb = fsm->isFileExistByUserFilePath(pathToFile);

        if(isFileExistInDb)
            item = TableModelFileMonitor::tableItemModifiedFileFrom(pathToFile);
        else
            item = TableModelFileMonitor::tableItemNewAddedFileFrom(pathToFile);

        return item;
    });

    watcher->setFuture(future);

//    auto item = TableModelFileMonitor::tableItemNewAddedFileFrom(pathToFile);
//    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotOnFileDeleted(const QString &pathToFile)
{
    auto *watcher = new QFutureWatcher<TableModelFileMonitor::TableItem>(this);
    resultSet.insert(watcher);
    QObject::connect(watcher, &QFutureWatcher<TableModelFileMonitor::TableItem>::finished,
                     this, &TabFileMonitor::slotRefreshTableViewFileMonitor);

    auto future = QtConcurrent::run([=]{

        TableModelFileMonitor::TableItem item;
        auto fsm = FileStorageManager::instance();

        bool isFileExistInDb = fsm->isFileExistByUserFilePath(pathToFile);

        if(isFileExistInDb)
            item = TableModelFileMonitor::tableItemDeletedFileFrom(pathToFile);

        return item;
    });

    watcher->setFuture(future);

//    auto item = TableModelFileMonitor::tableItemDeletedFileFrom(pathToFile);
//    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotOnFileMoved(const QString &pathToFile, const QString &oldFileName)
{
    QString pathToOldFile = QFileInfo(pathToFile).absolutePath();
    pathToOldFile = QDir::toNativeSeparators(pathToOldFile) + QDir::separator();
    pathToOldFile += oldFileName;

    auto *watcher = new QFutureWatcher<TableModelFileMonitor::TableItem>(this);
    resultSet.insert(watcher);
    QObject::connect(watcher, &QFutureWatcher<TableModelFileMonitor::TableItem>::finished,
                     this, &TabFileMonitor::slotRefreshTableViewFileMonitor);

    auto future = QtConcurrent::run([=]{
                      auto fsm = FileStorageManager::instance();
                      bool isFileExistInDb = fsm->isFileExistByUserFilePath(pathToOldFile);
                      return isFileExistInDb;

                  }).then(QtFuture::Launch::Inherit, [=](QFuture<bool> previous){
                          TableModelFileMonitor::TableItem item;

                          if(previous.result() == true)
                              item = TableModelFileMonitor::tableItemMovedFileFrom(pathToFile, oldFileName);

                          return item;
                      });

    watcher->setFuture(future);

//    auto item = TableModelFileMonitor::tableItemMovedFileFrom(pathToFile, oldFileName);
//    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotOnFileModified(const QString &pathToFile)
{
    auto *watcher = new QFutureWatcher<TableModelFileMonitor::TableItem>(this);
    resultSet.insert(watcher);
    QObject::connect(watcher, &QFutureWatcher<TableModelFileMonitor::TableItem>::finished,
                     this, &TabFileMonitor::slotRefreshTableViewFileMonitor);

    auto future = QtConcurrent::run([=]{
        auto fsm = FileStorageManager::instance();
        bool isFileExistInDb = fsm->isFileExistByUserFilePath(pathToFile);
        return isFileExistInDb;

    }).then(QtFuture::Launch::Inherit, [=](QFuture<bool> previous){
        TableModelFileMonitor::TableItem item;

        if(previous.result() == true)
            item = TableModelFileMonitor::tableItemModifiedFileFrom(pathToFile);

        return item;
    });

    watcher->setFuture(future);

//    auto item = TableModelFileMonitor::tableItemUpdatedFileFrom(pathToFile);
//    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotOnFileMovedAndModified(const QString &pathToFile, const QString &oldFileName)
{
    QString pathToOldFile = QFileInfo(pathToFile).absolutePath();
    pathToOldFile = QDir::toNativeSeparators(pathToOldFile) + QDir::separator();
    pathToOldFile += oldFileName;

    auto *watcher = new QFutureWatcher<TableModelFileMonitor::TableItem>(this);
    resultSet.insert(watcher);
    QObject::connect(watcher, &QFutureWatcher<TableModelFileMonitor::TableItem>::finished,
                     this, &TabFileMonitor::slotRefreshTableViewFileMonitor);

    auto future = QtConcurrent::run([=]{
                      auto fsm = FileStorageManager::instance();
                      bool isFileExistInDb = fsm->isFileExistByUserFilePath(pathToOldFile);
                      return isFileExistInDb;

                  }).then(QtFuture::Launch::Inherit, [=](QFuture<bool> previous){
                          TableModelFileMonitor::TableItem item;

                          if(previous.result() == true)
                              item = TableModelFileMonitor::tableItemMovedAndModifiedFileFrom(pathToFile, oldFileName);

                          return item;
                      });

    watcher->setFuture(future);

//    auto item = TableModelFileMonitor::tableItemMovedAndUpdatedFileFrom(pathToFile, oldFileName);
//    addRowToTableViewFileMonitor(item);
}

void TabFileMonitor::slotRefreshTableViewFileMonitor()
{
    for(QFutureWatcher<TableModelFileMonitor::TableItem> *watcher : qAsConst(resultSet))
    {
        if(watcher->isFinished())
        {
            addRowToTableViewFileMonitor(watcher->result());
            watcher->deleteLater();
            resultSet.remove(watcher);
        }
    }
}

void TabFileMonitor::addRowToTableViewFileMonitor(const TableModelFileMonitor::TableItem &item)
{
    if(item.status != TableModelFileMonitor::TableItemStatus::InvalidStatus)
    {
        auto tableModel = this->tableModelFileMonitor;
        tableModel->insertRows(0, 1, QModelIndex());

        QModelIndex index = tableModel->index(0, 0, QModelIndex());
        tableModel->setData(index, item.name, Qt::EditRole);

        index = tableModel->index(0, 1, QModelIndex());
        tableModel->setData(index, item.parentDirPath, Qt::EditRole);

        index = tableModel->index(0, 2, QModelIndex());
        tableModel->setData(index, item.oldName, Qt::EditRole);

        index = tableModel->index(0, 3, QModelIndex());
        tableModel->setData(index, item.itemType, Qt::EditRole);

        index = tableModel->index(0, 4, QModelIndex());
        tableModel->setData(index, item.status, Qt::EditRole);

        index = tableModel->index(0, 5, QModelIndex());
        tableModel->setData(index, item.timestamp, Qt::EditRole);

        ui->tableViewFileMonitor->resizeColumnsToContents();
    }
}
