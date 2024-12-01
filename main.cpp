#include "MainInterface.h"
#include <QtWidgets/QApplication>
#include <QDateTime> //���QDateTimeͷ�ļ�
#include <QPixmap>
#include <QSplashScreen>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QPixmap pixmap(":/MainInterface/res/cug.png");     //��ȡͼƬ
    QSplashScreen splash(pixmap); //
    splash.setWindowOpacity(0.8); // ���ô���͸����
    splash.show();
    QDateTime time = QDateTime::currentDateTime();
    QDateTime currentTime = QDateTime::currentDateTime(); //��¼��ǰʱ��
    while (time.secsTo(currentTime) <= 3) // 3Ϊ��Ҫ��ʱ������
    {
        currentTime = QDateTime::currentDateTime();
        a.processEvents();
    };
   
    MainInterface w;
    w.show(); // ���������ڣ���������ʾ

    splash.finish(&w);

    return a.exec();
}
