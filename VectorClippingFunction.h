#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_VectorClippingFunction.h"
#include <QDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QgsVectorLayer.h>
#include <QgsProject.h>
#include <QgsFeatureIterator.h>
#include <QgsFeature.h>
#include <QgsGeometry.h>
#include <QgsPoint.h>
#include <QgsRectangle.h>
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QgsMapCanvas.h>
#include <QgsMapTool.h>
#include <QgsVectorFileWriter.h>
#include <qgsmapmouseevent.h>
#include <QgsGeometry.h>
#include <QgsMapToolEdit.h>
#include <QgsMapToolEmitPoint.h>
#include <QgsMapToolCapture.h>
#include <QgsRubberBand.h>
#include <QgsRubberBand.h>
#include <ogr_geometry.h>
#include <ogrsf_frmts.h>
#include <qgsmaptool.h>
#include <qgspointxy.h>
#include <qgsmapcanvas.h>
#include <QPainter>
#include <QgsCircularString.h>
#include <QFileInfo>
#include <QStandardItem>
#include <QstringLiteral>
#include "StyleSheet.h"

class VectorClippingFunction : public QMainWindow
{
	Q_OBJECT

public:
	VectorClippingFunction(QgsMapCanvas* m_mapCanvas, QWidget* parent = nullptr);
	~VectorClippingFunction();
	bool clippingVector(const QString& outputPath);
	bool clippingTempVector(const QString& outputPath);

	QString strOutputFilePath;
	QString strInputFilePath;
	QString strInputClippFilePath;
	QList<QgsVectorLayer*> loadedLayers;
	QgsVectorLayer* inputLayer = nullptr;
	QgsVectorLayer* clippingLayer = nullptr;

private slots:
	void onInputVectorData();
	void onInputClippData();
	void onCreateClippingVector();
	void onActionClipping();

	void updateView();

signals:
	void beginDraw();

private:
	Ui::VectorClippingFunctionClass ui;
	void drawClippingVector(const QString& shapeType);
	void showVectorData(const QString& filePath);
	void setStyle();
	StyleSheet style;
	QgsMapCanvas* m_mapCanvas;
	QgsMapCanvas* mapCanvas;
	void showMapLayerBox();
	void showMapLayerList();
};



