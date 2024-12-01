#include "MainInterface.h"
#include <QtWidgets/QApplication>
#include <QDateTime> //添加QDateTime头文件
#include <QPixmap>
#include <QSplashScreen>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QPixmap pixmap(":/MainInterface/res/cug.png");     //读取图片
    QSplashScreen splash(pixmap); //
    splash.setWindowOpacity(0.8); // 设置窗口透明度
    splash.show();
    QDateTime time = QDateTime::currentDateTime();
    QDateTime currentTime = QDateTime::currentDateTime(); //记录当前时间
    while (time.secsTo(currentTime) <= 3) // 3为需要延时的秒数
    {
        currentTime = QDateTime::currentDateTime();
        a.processEvents();
    };
   
    MainInterface w;
    w.show(); // 隐藏主窗口，不立即显示

    splash.finish(&w);

    return a.exec();
}
