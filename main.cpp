#include <QApplication>
#include "menuwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 先打开菜单
    MenuWindow w;
    w.show();

    return a.exec();
}
