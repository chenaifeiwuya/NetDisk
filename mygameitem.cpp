#include "mygameitem.h"


myGameItem::myGameItem()
{
    this->setFixedSize(100,200);
    pLayout = new QHBoxLayout();
  //  pLayout->setMargin();
    pLabel = new QLabel();
    pLabel->setObjectName("GameIcon");
    pLabel->setText("this is a game");
    pLabel->show();
   // pLabel->setFixedSize(100*200);

    pLayout->addWidget(pLabel);
    this->setLayout(pLayout);
}

void myGameItem::slot_setInfo(FileInfo &info)
{

    m_info = info;

  //  this->setText(info.name);

    if(info.type == "game"){
        //设置游戏封面   游戏封面由独立的file构成
    }

    //不设置勾选框
}
