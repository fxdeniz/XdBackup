#include "TabFileMonitorLambdaFactory.h"
#include "Backend/FileStorageSubSystem/FileStorageManager.h"
#include "DataModels/TabFileMonitor/V2TableModelFileMonitor.h"

#include <QFileInfo>
#include <QUuid>
#include <QDir>

std::function<bool (QString)> TabFileMonitorLambdaFactory::lambdaIsFileExistInDb()
{
    return [](QString pathToFile) -> bool{
        auto fsm = FileStorageManager::instance();
        bool isFileExistInDb = fsm->isFileExistByUserFilePath(pathToFile);
        return isFileExistInDb;
    };
}

std::function<QSqlQuery (QString, QString)> TabFileMonitorLambdaFactory::lambdaFetchFileRowFromModelDb()
{
    return [] (QString connectionName, QString pathToFile) {

        QString newConnectionName = QUuid::createUuid().toString(QUuid::StringFormat::Id128);
        QSqlDatabase db = QSqlDatabase::cloneDatabase(connectionName, newConnectionName);
        db.open();

        QString queryTemplate = "SELECT * FROM %1 WHERE %2 = '%3';" ;
        queryTemplate = queryTemplate.arg(V2TableModelFileMonitor::TABLE_NAME,       // 1
                                          V2TableModelFileMonitor::COLUMN_NAME_PATH, // 2
                                          pathToFile);                               // 3

        QSqlQuery selectQuery(db);
        selectQuery.prepare(queryTemplate);
        selectQuery.exec();

        return selectQuery;
    };
}

std::function<void (QString, QString)> TabFileMonitorLambdaFactory::lambdaInsertModifiedFileIntoModelDb()
{
    return [](QString connectionName, QString pathToFile){
        QString newConnectionName = QUuid::createUuid().toString(QUuid::StringFormat::Id128);
        QSqlDatabase db = QSqlDatabase::cloneDatabase(connectionName, newConnectionName);
        db.open();

        QString queryTemplate = "INSERT INTO %1 (%2, %3, %4, %5, %6) "
                                "VALUES(:2, :3, :4, :5, :6);" ;

        queryTemplate = queryTemplate.arg(V2TableModelFileMonitor::TABLE_NAME,              // 1
                                          V2TableModelFileMonitor::COLUMN_NAME_NAME,        // 2
                                          V2TableModelFileMonitor::COLUMN_NAME_PARENT_DIR,  // 3
                                          V2TableModelFileMonitor::COLUMN_NAME_TYPE,        // 4
                                          V2TableModelFileMonitor::COLUMN_NAME_STATUS,      // 5
                                          V2TableModelFileMonitor::COLUMN_NAME_TIMESTAMP);  // 6

        QSqlQuery insertQuery(db);
        insertQuery.prepare(queryTemplate);

        QFileInfo info(pathToFile);
        auto parentDir = QDir::toNativeSeparators(info.absolutePath()) + QDir::separator();

        insertQuery.bindValue(":2", info.fileName());
        insertQuery.bindValue(":3", parentDir);
        insertQuery.bindValue(":4", V2TableModelFileMonitor::TableItemType::File);
        insertQuery.bindValue(":5", V2TableModelFileMonitor::TableItemStatus::Modified);
        insertQuery.bindValue(":6", QDateTime::currentDateTime());
        insertQuery.exec();
    };
}
