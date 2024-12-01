#pragma once

#include <QMainWindow>
#include "ui_Rasterize.h"
#include <qgsvectorlayer.h>
#include <qgslayertreeview.h>
#include "StyleSheet.h"
#include "qgsmapcanvas.h"

class Rasterize : public QMainWindow
{
	Q_OBJECT

public:
	Rasterize(QgsMapCanvas* m_mapCanvas, QWidget* parent = nullptr);
	~Rasterize();
private slots:
	void on_browseVectorFileButton_clicked();  // ���ʸ���ļ�
	void on_browseRasterFileButton_clicked();  // ������դ���ļ�·��
	void on_convertToRasterButton_clicked();   // ִ��ʸ��תդ��
	void onCurrentItemChanged(); 
	void updateComboBox();

private:
	Ui::RasterizeClass ui;

	QgsMapCanvas* m_mapCanvas;

	QList<QgsVectorLayer*> m_vectorLayers; // �洢ͼ��ĳ�Ա����

	QString m_filePath;

	void setStyle();
	StyleSheet style;
	void loadVectorLayer(QgsVectorLayer* vectorLayer);
	void updateFieldComboBox(QgsVectorLayer* vectorLayer);

	QgsLayerTreeView* m_layerTreeView;

	QgsVectorLayer* m_vectorLayer = nullptr;    //ʸ��ͼ��

	QString m_outputFilePath;
};
