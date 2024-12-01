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
    setWindowTitle(QStringLiteral("ʸ��ͼ��༭"));

    setStyle();

    // ������ɫѡ��ť�Ĳ�
    connect(ui.colorButton, &QPushButton::clicked, this, [this] {
        QColor color = QColorDialog::getColor(Qt::white, this, QStringLiteral("ѡ����ɫ"));
        if (color.isValid()) {
            ui.colorButton->setStyleSheet("background-color: " + color.name() + ";");
        }
        });

    // ����͸���Ȼ����ֵ�仯�ź����
    connect(ui.opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        ui.opacityLabel->setText(QString::number(value / 100.0));  // ����͸���ȱ�ǩ
        });

    connect(ui.okButton, &QPushButton::clicked, [&]() {
        accept(); // ���ܶԻ��򣬹ر�
        });

    connect(ui.cancelButton, &QPushButton::clicked, [&]() {
        reject(); // ���ܶԻ��򣬹ر�
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

