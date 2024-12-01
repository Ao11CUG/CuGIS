#include "LayerStyle.h"
#include <QColorDialog>
#include <QSpinBox>
#include <QSlider>
#include<qgssymbol.h>
LayerStyle::LayerStyle(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    setWindowIcon(QIcon(":/MainInterface/res/openVector.png"));
    setWindowTitle(QStringLiteral("矢量图层编辑"));

    setStyle();

    // 设置颜色选择按钮的槽
    connect(ui.colorButton, &QPushButton::clicked, this, [this] {
        QColor color = QColorDialog::getColor(Qt::white, this, QStringLiteral("选择颜色"));
        if (color.isValid()) {
            ui.colorButton->setStyleSheet("background-color: " + color.name() + ";");
        }
        });

    // 连接透明度滑块的值变化信号与槽
    connect(ui.opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        ui.opacityLabel->setText(QString::number(value / 100.0));  // 设置透明度标签
        });

    connect(ui.okButton, &QPushButton::clicked, [&]() {
        accept(); // 接受对话框，关闭
        });

    connect(ui.cancelButton, &QPushButton::clicked, [&]() {
        reject(); // 接受对话框，关闭
        });
}

LayerStyle::~LayerStyle() {}

void LayerStyle::setStyle() {
    style.setConfirmButton(ui.colorButton);

    style.setConfirmButton(ui.okButton);

    style.setConfirmButton(ui.cancelButton);

    style.setSpinBox(ui.sizeSpinBox);
}

QColor LayerStyle::getColor() const
{
    return ui.colorButton->palette().button().color();
}

int LayerStyle::getSize() const
{
    return ui.sizeSpinBox->value();
}

double LayerStyle::getOpacity() const
{
    return ui.opacitySlider->value() / 100.0;
}

