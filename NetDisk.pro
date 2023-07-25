QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
include(./netapi/netapi.pri)
include(./md5/md5.pri)
include(./sqlapi/sqlapi.pri)
INCLUDEPATH += ./netapi/   #添加依赖路径
INCLUDEPATH += ./md5/
INCLUDEPATH += ./sqlapi/

SOURCES += \
    ckernel.cpp \
    main.cpp \
    maindialog.cpp \
    mainwindow.cpp \
    mytablewidgetitem.cpp

HEADERS += \
    ckernel.h \
    maindialog.h \
    mainwindow.h \
    mytablewidgetitem.h

FORMS += \
    maindialog.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Source.qrc \
    images.qrc

DISTFILES += \
    C:/Users/00/Pictures/11b91037ac6e4c278d48a75b0a87d550.jpeg \
    C:/Users/00/Pictures/AA/txfb.jpg \
