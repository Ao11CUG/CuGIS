//Ê¸Á¿Í¼²ãË«»÷
#pragma once

#include <QDialog>
#include <QColor>
#include "ui_LayerStyle.h"
#include <QSpinBox>
#include <QSlider>
#include <qgsvectorlayer.h>
#include "StyleSheet.h"

class LayerStyle : public QDialog
{
    Q_OBJECT

public:
    LayerStyle(QWidget* parent = nullptr);
    ~LayerStyle();

    QColor getColor() const;
    int getSize() const;
    double getOpacity() const;


private:
    Ui::LayerStyleClass ui;
    StyleSheet style;
    void setStyle();
};
