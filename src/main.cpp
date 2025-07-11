#include "../include/mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置应用程序信息
    QApplication::setApplicationName("SecureFileManager");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("XuZihao");
    
    // 创建并显示主窗口
    MainWindow w;
    w.show();
    
    return a.exec();
}