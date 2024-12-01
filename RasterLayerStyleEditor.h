//栅格图层双击
#pragma once

#include <QDialog>
#include <QColor>
#include <QSlider>
#include <QComboBox>
#include "ui_RasterLayerStyleEditor.h"
#include <qgsrasterlayer.h>
#include "StyleSheet.h"

class RasterLayerStyleEditor : public QDialog
{
    Q_OBJECT

public:
    RasterLayerStyleEditor(QgsRasterLayer* rasterLayer, QWidget* parent = nullptr);
    ~RasterLayerStyleEditor();

    double getOpacity() const;
    QString getColorRamp() const;  // 获取选择的色表
    int getBandLevel()const;
    void applyChanges();//应用按钮

private:
    Ui::RasterLayerStyleEditorClass ui;
    StyleSheet style;
    void setStyle();
    QgsRasterLayer* m_rasterLayer;
};