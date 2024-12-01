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
	void on_browseVectorFileButton_clicked();  // 浏览矢量文件
	void on_browseRasterFileButton_clicked();  // 浏览输出栅格文件路径
	void on_convertToRasterButton_clicked();   // 执行矢量转栅格
	void onCurrentItemChanged(); 
	void updateComboBox();

private:
	Ui::RasterizeClass ui;

	QgsMapCanvas* m_mapCanvas;

	QList<QgsVectorLayer*> m_vectorLayers; // 存储图层的成员变量

	QString m_filePath;

	void setStyle();
	StyleSheet style;
	void loadVectorLayer(QgsVectorLayer* vectorLayer);
	void updateFieldComboBox(QgsVectorLayer* vectorLayer);

	QgsLayerTreeView* m_layerTreeView;

	QgsVectorLayer* m_vectorLayer = nullptr;    //矢量图层

	QString m_outputFilePath;
};
