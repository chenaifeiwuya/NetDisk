#ifndef MYGAMEITEM_H
#define MYGAMEITEM_H

#include <QTableWidgetItem>
#include <packdef.h>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

class MainWindow;
class myGameItem: public QTableWidget
{
public:
    myGameItem();
public slots:
    void slot_setInfo(FileInfo & info);     //这里的FileInfo为游戏信息
private:
    FileInfo m_info;
    QHBoxLayout *pLayout;   // = new QHBoxLayout();
    QLabel *pLabel;// = new QLabel();
    friend class MainWindow;
};

#endif // MYGAMEITEM_H



