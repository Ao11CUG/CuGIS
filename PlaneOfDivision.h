#pragma once
#include <QtWidgets/QMainWindow>
#include "ui_PlaneOfDivision.h"
#include <QPushButton>
#include <QString>
#include <QgsVectorLayer.h>
#include <QgsMapCanvas.h>
#include <QMessageBox>
#include <QFileDialog>
#include <QStringLiteral>
#include <QgsMapToolIdentifyFeature.h>
#include <QgsMapToolCapture.h>
#include <QgsGeometry.h>
#include <QgsFeature.h>
#include <QgsVectorLayer.h>
#include <QgsFeatureRequest.h>
#include <QgsField.h>
#include <QgsProject.h>
#include <QgsMapTool.h>
#include <QSet>
#include <QgsRubberBand.h>
#include <QgsMapMouseEvent.h>
#include <QgsMapTool.h>
#include <QStandardItemModel.h>
#include <QInputDialog.h>
#include <QgsVectorFileWriter.h>
#include "DrawGeometryTool.h"
#include "StyleSheet.h"

class PlaneOfDivision : public QMainWindow
{
	Q_OBJECT

public:
	PlaneOfDivision(QgsMapCanvas* m_mapCanvas, QWidget* parent = nullptr);
	~PlaneOfDivision();

private slots:
	void onInputVectorDataClicked();
	void onCreateDivisionClicked();
	void onActionDivisionClicked();
	void onFeatureIdentified(const QgsFeature& feature);
	void highlightSelectedFeatures(const QList<QgsFeature>& features);
	void updateView();

private:
	Ui::PlaneOfDivisionClass ui;

	StyleSheet style;
	void setStyle();

	QgsMapCanvas* m_mapCanvas;

	QString strOutputFilePath;
	QString strInputFilePath;

	void showVectorData(const QString& filePath);
	void showDivisionData();
	void actionDivision();
	void createDivisionPolygon(const QString& shapeType);

	QgsVectorLayer* inputLayer = nullptr;
	bool isSelecting = false;  

	QgsVectorLayer* DivisionLayer = nullptr;

	QgsMapTool* drawPolygonTool = nullptr;  

	QgsMapCanvas* mapCanvas = nullptr;

	QgsMapToolCapture* divisionTool = nullptr;

	QgsRubberBand* rubberBand = nullptr;

	QgsMapToolIdentifyFeature* identifyTool = nullptr;

	QList<QgsFeature> selectedFeatures;

	QList<QgsVectorLayer*> loadedLayers;

	void showMapLayerBox();
};
