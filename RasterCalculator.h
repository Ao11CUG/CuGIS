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
    // ѡ��դ��ͼ��
    void on_selectRasterLayersButton_clicked();

    // ִ��դ�����
    void on_calculateButton_clicked();

    // ѡ������ļ�·��
    void on_selectOutputFileButton_clicked();

    // ˫�� rasterLayerlistView �е�դ�����ƣ�������ӵ����ʽ��
    void on_rasterLayerlistView_doubleClicked(const QModelIndex& index);

    // �������ӵ����
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

    // ��ʼ�������㷨
    void initializeProcessing();

    // QStringListModel ���ڹ���դ��ͼ������
    QStringListModel* rasterLayerModel = nullptr;

    // UI Ԫ��
    Ui::RasterCalculatorClass ui;

    StyleSheet style;
    void setStyle();

    // �洢դ��ͼ�����ƺ�դ��ͼ�����
    QStringList m_rasterLayerNames;
    QList<QgsRasterLayer*> m_rasterLayers;
    QString m_outputFilePath = nullptr;
};

