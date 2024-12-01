#include <qgsmapcanvas.h>
#include <qgsmaplayer.h>
#include <qgsproject.h>
#include <QVBoxLayout>
#include <QWidget>

class EagleEye : public QWidget {
    Q_OBJECT

public:
    EagleEye(QgsMapCanvas* mainCanvas, QWidget* parent = nullptr)
        : QWidget(parent), mainMapCanvas(mainCanvas) {

        // ���ò���
        QVBoxLayout* layout = new QVBoxLayout(this);

        // ����ӥ�ۻ���
        eagleEyeCanvas = new QgsMapCanvas();
        eagleEyeCanvas->setCanvasColor(Qt::black);

        // �����������ķ�Χ����ź�
        connect(mainMapCanvas, &QgsMapCanvas::extentsChanged, this, &EagleEye::updateEagleEye);

        // ��ӥ�ۻ�����ӵ�����
        layout->addWidget(eagleEyeCanvas);
        setLayout(layout);
    }

public slots:
    void updateEagleEye() {
        // ����ӥ�ۻ����ķ�Χ��ͼ��
        eagleEyeCanvas->setExtent(mainMapCanvas->extent());
        eagleEyeCanvas->setLayers(mainMapCanvas->layers());
    }

public:
    QgsMapCanvas* eagleEyeCanvas; // ӥ�ۻ���ָ��
    QgsMapCanvas* mainMapCanvas; // ����ͼ����ָ��
};

