#ifndef TABFILEMONITOR_H
#define TABFILEMONITOR_H

#include <QWidget>

#include "ItemDelegates/TabFileMonitor/ComboBoxItemDelegateFileAction.h"
#include "ItemDelegates/TabFileMonitor/ComboBoxItemDelegateNote.h"
#include "DataModels/TabFileMonitor/TableModelFileMonitor.h"

namespace Ui {
class TabFileMonitor;
}

class TabFileMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit TabFileMonitor(QWidget *parent = nullptr);
    ~TabFileMonitor();

public slots:
    void slotOnPredictedFileNotFound(const QString &pathToFile);

private:
    Ui::TabFileMonitor *ui;
    TableModelFileMonitor *tableModelFileMonitor;
    ComboBoxItemDelegateNote *comboBoxItemDelegateNote;
    ComboBoxItemDelegateFileAction *comboBoxItemDelegateFileAction;

};

#endif // TABFILEMONITOR_H
