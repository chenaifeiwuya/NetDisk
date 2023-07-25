#ifndef MYTABLEWIDGETITEM_H
#define MYTABLEWIDGETITEM_H

#include <QTableWidgetItem>
#include <packdef.h>

class MainWindow;
class MyTableWidgetItem : public QTableWidgetItem
{
public:
    MyTableWidgetItem();
public slots:
    void slot_setInfo(FileInfo & info);

private:
    FileInfo m_info;
    friend class MainWindow;
};

#endif // MYTABLEWIDGETITEM_H
