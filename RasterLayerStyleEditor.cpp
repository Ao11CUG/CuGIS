#include "RasterLayerStyleEditor.h"
#include "ui_RasterLayerStyleEditor.h"
#include <QColorDialog>
#include <QSlider>
#include <QComboBox>
#include <QgsRasterShader.h>
#include <QgsColorRampShader.h>
#include <QgsRasterRenderer.h>
#include <qgsrasterlayer.h>
#include <qgsrastershader.h>
#include <qgscolorrampshader.h>
#include <qgscolorramp.h>
#include <qgssinglebandpseudocolorrenderer.h> 

RasterLayerStyleEditor::RasterLayerStyleEditor(QgsRasterLayer* rasterLayer, QWidget* parent)
    : QDialog(parent), m_rasterLayer(rasterLayer)
{
    ui.setupUi(this);

    setStyle();

    setWindowIcon(QIcon(":/MainInterface/res/openRaster.png"));
    setWindowTitle(QStringLiteral("դ��ͼ��༭"));

    // ����͸���Ȼ���
    connect(ui.opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        ui.opacityLabel->setText(QString::number(value / 100.0));  // ����͸���ȱ�ǩ
        });

    // ����Ӧ�ð�ť�ĵ���¼�
    connect(ui.applyButton, &QPushButton::clicked, this, &RasterLayerStyleEditor::applyChanges);

    // ����ɫ��ѡ����ṩ����������ɫ��ѡ��
    ui.colorRampComboBox->addItem(QStringLiteral("Greyscale(�Ҷ�)"));
    ui.colorRampComboBox->addItem(QStringLiteral("Viridis(����ɫ������ɫ)"));
    ui.colorRampComboBox->addItem(QStringLiteral("Inferno����ɫ����ɫ��"));
    ui.colorRampComboBox->addItem(QStringLiteral("Magma����ɫ�����ɫ��"));

    // ����դ��ͼ����������ã���ʼ��͸���Ⱥ�ɫ��
    ui.opacitySlider->setValue(m_rasterLayer->renderer()->opacity() * 100); // ����͸������0��1�ķ�Χ
    // Ĭ��ѡ���һ��ɫ��
    ui.colorRampComboBox->setCurrentIndex(0);

}

RasterLayerStyleEditor::~RasterLayerStyleEditor() {}

void RasterLayerStyleEditor::setStyle() {
    style.setConfirmButton(ui.applyButton);

    style.setComboBox(ui.colorRampComboBox);

    style.setSpinBox(ui.spinBoxBandLevel);
}

double RasterLayerStyleEditor::getOpacity() const
{
    return ui.opacitySlider->value() / 100.0;
}

int RasterLayerStyleEditor::getBandLevel()const {
    return ui.spinBoxBandLevel->value();
}

QString RasterLayerStyleEditor::getColorRamp() const
{
    return ui.colorRampComboBox->currentText();
}

// Ӧ�����õ�դ��ͼ��
void RasterLayerStyleEditor::applyChanges()
{
    // ��ȡ��ǰ���õ�͸���Ⱥ�ɫ��
    double opacity = getOpacity();
    QString colorRamp = getColorRamp();

    QgsRasterLayer* rasterLayer = m_rasterLayer;
    QgsRasterDataProvider* dataProvider = rasterLayer->dataProvider();
    QgsRasterBandStats stats = dataProvider->bandStatistics(1, QgsRasterBandStats::All);
    double minValue = stats.minimumValue;
    double maxValue = stats.maximumValue;

    // ����դ��ͼ�����Ⱦ��
    QgsRasterShader* shader = new QgsRasterShader();
    QgsColorRampShader* colorRampShader = new QgsColorRampShader();

    // ����ɫ��������ѡ���ɫ��
    QgsColorRamp* gradientRamp = nullptr;
    if (colorRamp == QStringLiteral("Greyscale(�Ҷ�)")) {
        gradientRamp = new QgsGradientColorRamp(QColor(0, 0, 0), QColor(255, 255, 255));
    }
    else if (colorRamp == QStringLiteral("Viridis(����ɫ������ɫ)")) {
        gradientRamp = new QgsGradientColorRamp(QColor(68, 1, 84), QColor(253, 231, 37));
    }
    else if (colorRamp == QStringLiteral("Inferno����ɫ����ɫ��")) {
        gradientRamp = new QgsGradientColorRamp(QColor(0, 0, 0), QColor(255, 70, 0));
    }
    else if (colorRamp == QStringLiteral("Magma����ɫ�����ɫ��")) {
        gradientRamp = new QgsGradientColorRamp(QColor(0, 0, 0), QColor(158, 1, 66));
    }


    if (gradientRamp) {
        // ��̬���ɸ�����Ŀ
        QList<QgsColorRampShader::ColorRampItem> colorRampItems;

        // �ּ�����
        int numItems = getBandLevel();
        for (int i = 0; i <= numItems; ++i) {
            double value = minValue + (maxValue - minValue) * i / numItems; // ���ȷֲ�
            colorRampItems.append(QgsColorRampShader::ColorRampItem(value, gradientRamp->color(value)));
        }

        // ����ɫ����Ŀ��ɫ����ɫ��
        colorRampShader->setColorRampItemList(colorRampItems);
        shader->setRasterShaderFunction(colorRampShader);
    }


    // ����դ��ͼ�����Ⱦ��
    QgsSingleBandPseudoColorRenderer* renderer = new QgsSingleBandPseudoColorRenderer(
        m_rasterLayer->dataProvider(), 1, shader);  // ����դ��ͼ����Ⱦ��

    // ����͸����
    renderer->setOpacity(opacity);

    // ����դ��ͼ����Ⱦ��
    m_rasterLayer->setRenderer(renderer);

    // Ӧ����Ⱦ����ˢ��ͼ��
    m_rasterLayer->triggerRepaint();
}
