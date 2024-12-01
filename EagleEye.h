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

        // 设置布局
        QVBoxLayout* layout = new QVBoxLayout(this);

        // 创建鹰眼画布
        eagleEyeCanvas = new QgsMapCanvas();
        eagleEyeCanvas->setCanvasColor(Qt::black);

        // 连接主画布的范围变更信号
        connect(mainMapCanvas, &QgsMapCanvas::extentsChanged, this, &EagleEye::updateEagleEye);

        // 将鹰眼画布添加到布局
        layout->addWidget(eagleEyeCanvas);
        setLayout(layout);
    }

public slots:
    void updateEagleEye() {
        // 更新鹰眼画布的范围和图层
        eagleEyeCanvas->setExtent(mainMapCanvas->extent());
        eagleEyeCanvas->setLayers(mainMapCanvas->layers());
    }

public:
    QgsMapCanvas* eagleEyeCanvas; // 鹰眼画布指针
    QgsMapCanvas* mainMapCanvas; // 主地图画布指针
};

