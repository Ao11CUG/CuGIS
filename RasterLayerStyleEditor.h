//դ��ͼ��˫��
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
    QString getColorRamp() const;  // ��ȡѡ���ɫ��
    int getBandLevel()const;
    void applyChanges();//Ӧ�ð�ť

private:
    Ui::RasterLayerStyleEditorClass ui;
    StyleSheet style;
    void setStyle();
    QgsRasterLayer* m_rasterLayer;
};