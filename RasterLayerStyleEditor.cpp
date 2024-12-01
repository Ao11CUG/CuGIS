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
    setWindowTitle(QStringLiteral("栅格图层编辑"));

    // 设置透明度滑块
    connect(ui.opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        ui.opacityLabel->setText(QString::number(value / 100.0));  // 更新透明度标签
        });

    // 连接应用按钮的点击事件
    connect(ui.applyButton, &QPushButton::clicked, this, &RasterLayerStyleEditor::applyChanges);

    // 设置色表选择框，提供几个常见的色表选择
    ui.colorRampComboBox->addItem(QStringLiteral("Greyscale(灰度)"));
    ui.colorRampComboBox->addItem(QStringLiteral("Viridis(深蓝色到亮黄色)"));
    ui.colorRampComboBox->addItem(QStringLiteral("Inferno（黑色到红色）"));
    ui.colorRampComboBox->addItem(QStringLiteral("Magma（黑色到深红色）"));

    // 根据栅格图层的现有设置，初始化透明度和色表
    ui.opacitySlider->setValue(m_rasterLayer->renderer()->opacity() * 100); // 假设透明度是0到1的范围
    // 默认选择第一个色表
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

// 应用设置到栅格图层
void RasterLayerStyleEditor::applyChanges()
{
    // 获取当前设置的透明度和色表
    double opacity = getOpacity();
    QString colorRamp = getColorRamp();

    QgsRasterLayer* rasterLayer = m_rasterLayer;
    QgsRasterDataProvider* dataProvider = rasterLayer->dataProvider();
    QgsRasterBandStats stats = dataProvider->bandStatistics(1, QgsRasterBandStats::All);
    double minValue = stats.minimumValue;
    double maxValue = stats.maximumValue;

    // 创建栅格图层的渲染器
    QgsRasterShader* shader = new QgsRasterShader();
    QgsColorRampShader* colorRampShader = new QgsColorRampShader();

    // 设置色带（根据选择的色表）
    QgsColorRamp* gradientRamp = nullptr;
    if (colorRamp == QStringLiteral("Greyscale(灰度)")) {
        gradientRamp = new QgsGradientColorRamp(QColor(0, 0, 0), QColor(255, 255, 255));
    }
    else if (colorRamp == QStringLiteral("Viridis(深蓝色到亮黄色)")) {
        gradientRamp = new QgsGradientColorRamp(QColor(68, 1, 84), QColor(253, 231, 37));
    }
    else if (colorRamp == QStringLiteral("Inferno（黑色到红色）")) {
        gradientRamp = new QgsGradientColorRamp(QColor(0, 0, 0), QColor(255, 70, 0));
    }
    else if (colorRamp == QStringLiteral("Magma（黑色到深红色）")) {
        gradientRamp = new QgsGradientColorRamp(QColor(0, 0, 0), QColor(158, 1, 66));
    }


    if (gradientRamp) {
        // 动态生成更多条目
        QList<QgsColorRampShader::ColorRampItem> colorRampItems;

        // 分级个数
        int numItems = getBandLevel();
        for (int i = 0; i <= numItems; ++i) {
            double value = minValue + (maxValue - minValue) * i / numItems; // 均匀分布
            colorRampItems.append(QgsColorRampShader::ColorRampItem(value, gradientRamp->color(value)));
        }

        // 设置色带条目到色带着色器
        colorRampShader->setColorRampItemList(colorRampItems);
        shader->setRasterShaderFunction(colorRampShader);
    }


    // 创建栅格图层的渲染器
    QgsSingleBandPseudoColorRenderer* renderer = new QgsSingleBandPseudoColorRenderer(
        m_rasterLayer->dataProvider(), 1, shader);  // 创建栅格图层渲染器

    // 设置透明度
    renderer->setOpacity(opacity);

    // 设置栅格图层渲染器
    m_rasterLayer->setRenderer(renderer);

    // 应用渲染器并刷新图层
    m_rasterLayer->triggerRepaint();
}
