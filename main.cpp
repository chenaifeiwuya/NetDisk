#include <QApplication>
#include "cKernel.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    Maindialog w;
//    w.show();
    auto kernel=cKernel::GetInstance();
    return a.exec();
}
