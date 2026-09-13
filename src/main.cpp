#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 設置應用程式信息
    app.setApplicationName("OTL Generator");
    app.setApplicationVersion("1.0.0");
    app.setApplicationDisplayName("OTL 一次性連結生成器");
    
    // 創建並顯示主窗口
    MainWindow window;
    window.show();
    
    return app.exec();
}
