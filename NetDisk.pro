TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

QT += mysql
INCLUDEPATH += ./serverNet/include/
LIBS += -lpthread -lmysqlclient
SOURCES += \
        serverNet/src/Mysql.cpp \
        serverNet/src/TCPKernel.cpp \
        serverNet/src/Thread_pool.cpp \
        serverNet/src/block_epoll_net.cpp \
        serverNet/src/clogic.cpp \
        serverNet/src/err_str.cpp \
        serverNet/src/main.cpp

HEADERS += \
    ../../../../mnt/hgfs/linux共享文件夹/0608LinuxNetServer/0608LinuxNetServer/threadpool.h \
    serverNet/include/Mysql.h \
    serverNet/include/TCPKernel.h \
    serverNet/include/Thread_pool.h \
    serverNet/include/block_epoll_net.h \
    serverNet/include/clogic.h \
    serverNet/include/err_str.h \
    serverNet/include/packdef.h
