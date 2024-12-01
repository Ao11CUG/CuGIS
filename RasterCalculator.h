#pragma once
#include <QMainWindow>
#include "ui_RasterCalculator.h"
#include <QgsRasterLayer.h>
#include <QgsCoordinateReferenceSystem.h>
#include <QgsProcessingAlgorithm.h>
#include <QgsProcessingContext.h>
#include <QgsProcessingFeedback.h>
#include <QStringListModel>
#include "StyleSheet.h"
#include "qgsmapcanvas.h"

class RasterCalculator : public QMainWindow
{
    Q_OBJECT

public:
    explicit RasterCalculator(QgsMapCanvas* m_mapCanvas, QWidget* parent = nullptr);
    ~RasterCalculator();

private slots:
    // 选择栅格图层
    void on_selectRasterLayersButton_clicked();

    // 执行栅格计算
    void on_calculateButton_clicked();

    // 选择输出文件路径
    void on_selectOutputFileButton_clicked();

    // 双击 rasterLayerlistView 中的栅格名称，将其添加到表达式中
    void on_rasterLayerlistView_doubleClicked(const QModelIndex& index);

    // 计算算子的添加
    void on_pushButton_add_clicked();
    void on_pushButton_subtract_clicked();
    void on_pushButton_multiply_clicked();
    void on_pushButton_divide_clicked();
    void on_pushButton_log10_clicked();
    void on_pushButton_ln_clicked();
    void on_pushButton_index_clicked();
    void on_pushButton_if_clicked();
    void on_pushButton_openBracket_clicked();
    void on_pushButton_closeBracket_clicked();
    void on_pushButton_equal_clicked();
    void on_pushButton_notEqual_clicked();

    void updateListview();

private:
    QgsMapCanvas* m_mapCanvas;

    // 初始化处理算法
    void initializeProcessing();

    // QStringListModel 用于管理栅格图层名称
    QStringListModel* rasterLayerModel = nullptr;

    // UI 元素
    Ui::RasterCalculatorClass ui;

    StyleSheet style;
    void setStyle();

    // 存储栅格图层名称和栅格图层对象
    QStringList m_rasterLayerNames;
    QList<QgsRasterLayer*> m_rasterLayers;
    QString m_outputFilePath = nullptr;
};

