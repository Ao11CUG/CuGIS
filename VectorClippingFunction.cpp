#include "VectorClippingFunction.h"
#include <ui_VectorClippingFunction.h>
#include <QMessageBox>
#include <QFileDialog>
#include <QgsVectorLayer.h>
#include <QgsProject.h>
#include <QgsmapCanvas.h>
#include <QgsMapTool.h>
#include <QInputDialog>
#include "DrawGeometryTool.h"

VectorClippingFunction::VectorClippingFunction(QgsMapCanvas* m_mapCanvas, QWidget* parent)
	: QMainWindow(parent), m_mapCanvas(m_mapCanvas)
{
	ui.setupUi(this);

	setWindowIcon(QIcon(":/MainInterface/res/vectorTailor.png"));
	setWindowTitle(QStringLiteral("ʸ���ü�����"));

	// ȷ����Ŀ�Ѽ���
	QgsProject::instance();
	
	setStyle();

	// ���� QgsmapCanvas ������Ƕ�뵽 QScrollArea ��
	mapCanvas = new QgsMapCanvas();
	mapCanvas->setCanvasColor(Qt::white);  // ���ñ�����ɫ

	// �� QgsmapCanvas ���� QScrollArea ��
	ui.viewArea->setWidget(mapCanvas);  // �� mapCanvas ��Ϊ QScrollArea ������

	// �����ź����
	connect(ui.inputVectorData, &QPushButton::clicked, this, &VectorClippingFunction::onInputVectorData);
	connect(ui.inputClippData, &QPushButton::clicked, this, &VectorClippingFunction::onInputClippData);
	connect(ui.createClippingVector, &QPushButton::clicked, this, &VectorClippingFunction::onCreateClippingVector);
	connect(ui.actionClipping, &QPushButton::clicked, this, &VectorClippingFunction::onActionClipping);

	updateView();

	// �����������ķ�Χ����ź�
	connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &VectorClippingFunction::updateView);
	connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &VectorClippingFunction::showMapLayerBox);

	showMapLayerList();
	showMapLayerBox();
}

VectorClippingFunction::~VectorClippingFunction()
{
}

void VectorClippingFunction::updateView() {
	mapCanvas->setExtent(m_mapCanvas->extent());
	mapCanvas->setLayers(m_mapCanvas->layers());
}

void VectorClippingFunction::setStyle() {
	style.setConfirmButton(ui.actionClipping);
	
	style.setConfirmButton(ui.createClippingVector);

	style.setCommonButton(ui.inputClippData);

	style.setCommonButton(ui.inputVectorData);

	style.setLineEdit(ui.inputLineEdit);

	style.setLineEdit(ui.clipLineEdit);

	style.setComboBox(ui.clippiingVectorBox);

	style.setComboBox(ui.inputVectorBox);
}

void VectorClippingFunction::showVectorData(const QString& filePath)
{
	// ���� QgsVectorLayer ������ʸ������
	QgsVectorLayer* vectorLayer = new QgsVectorLayer(filePath, QStringLiteral("����ʸ����Ҫ��"), "ogr");

	if (!vectorLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("����ʸ����ʧ�ܣ�"));
		return;
	}

	// ����ͼ����ӵ� QgsProject ��
	QgsProject::instance()->addMapLayer(vectorLayer);

	// ��ͼ����ӵ��Ѽ���ͼ���б���
	loadedLayers.append(vectorLayer);  // ��ӵ� QList ��

	// ��� mapCanvas û������ͼ�㣬��ʼ��ͼ��
	if (mapCanvas->layers().isEmpty()) {
		mapCanvas->setLayers({ vectorLayer });
	}
	else {
		// �������ͼ�㣬������ͼ��
		QList<QgsMapLayer*> layers = mapCanvas->layers();
		layers.append(vectorLayer);  // �����ͼ��
		mapCanvas->setLayers(layers);  // ����ͼ���б�
	}

	// �Զ�������ͼ��ʾ��Χ����Ӧ����Ҫ��
	mapCanvas->zoomToFullExtent();

	// ˢ�µ�ͼ
	mapCanvas->refresh();
}

void VectorClippingFunction::onActionClipping()
{
	// ��ȡѡ�е�����
	int currentIndex = ui.inputVectorBox->currentIndex();

	if (currentIndex < 0 || currentIndex >= loadedLayers.size()) {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("ʸ��ͼ�㲻���ڣ�"));
		return;
	}
	if (strInputFilePath.isEmpty()) {
		// ���ڵ�ǰ������ȡѡ�е�ͼ��
		inputLayer = loadedLayers[currentIndex];
	}

	// ��ȡѡ�е�����
	int currentClipIndex = ui.clippiingVectorBox->currentIndex();

	if (currentClipIndex < 0 || currentClipIndex >= loadedLayers.size()) { 
		QMessageBox::critical(this, tr("Error"), QStringLiteral("ʸ��ͼ�㲻���ڣ�"));
		return;
	}
	if (strInputClippFilePath.isEmpty() && !clippingLayer) {
		// ���ڵ�ǰ������ȡѡ�е�ͼ��
		clippingLayer = loadedLayers[currentIndex];
	}
	
	// ȷ�����ü���ͼ��Ͳü�ͼ���Ѽ���
	if (!inputLayer || !clippingLayer) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("�����ͼ��в㶼�����ڼ���֮ǰ����!"));
		return;
	}

	// ���ļ��Ի������û�ѡ�񱣴�·��
	strOutputFilePath = QFileDialog::getSaveFileName(this, QStringLiteral("����ü�ͼ����"), "", QStringLiteral("Shapefiles (*.shp)"));

	if (strOutputFilePath.isEmpty()) {
		return; // ����û�û��ѡ��·����ֱ�ӷ���
	}

	// ���òü���������������浽ָ��·��
	if (!clippingVector(strOutputFilePath)) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("�������ͼ�㵽ָ��·��ʧ�ܣ�"));
	}
	else {
		QMessageBox::information(this, QStringLiteral("�ɹ�"), QStringLiteral("�����е�ͼ���ѳɹ�����Ϊ %1").arg(strOutputFilePath));
	}
	// ���زü������ canvas
	QgsVectorLayer* clippedLayer = new QgsVectorLayer(strOutputFilePath, "Clipped Result", "ogr");
	if (!clippedLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("���ؼ��в�ʧ�ܣ�"));
		delete clippedLayer;
		return;
	}

	// ���ü������ӵ� QgsProject ��
	QgsProject::instance()->addMapLayer(clippedLayer);

	// ���µ�ͼ��Χ����Ӧ�ü����
	mapCanvas->setExtent(clippedLayer->extent());
	mapCanvas->refresh();
}

void VectorClippingFunction::onInputVectorData()
{
	// ���ļ��Ի���ѡ��ʸ���ļ�
	strInputFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("ѡ��ʸ����Ҫ��"), "", QStringLiteral("Shapefiles (*.shp);;All Files (*)"));

	if (!strInputFilePath.isEmpty()) {
		// ���ر��ü�ͼ��
		inputLayer = new QgsVectorLayer(strInputFilePath, "Input Layer", "ogr");
		if (!inputLayer->isValid()) {
			QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("��������ʸ����ʧ�ܣ�"));
			delete inputLayer;
			inputLayer = nullptr;
			return;
		}

		ui.inputLineEdit->setText(strInputFilePath);

		// ��ͼ����ӵ� QgsProject ��
		QgsProject::instance()->addMapLayer(inputLayer);

		// ��ӵ� mapCanvas ��ͼ���б���
		QList<QgsMapLayer*> layers = mapCanvas->layers();
		layers.append(inputLayer);
	}
}

void VectorClippingFunction::onInputClippData()
{
	// ���ļ��Ի���ѡ��ü�ʸ���ļ�
	strInputClippFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("ѡ��ü���Ҫ�أ�"), "", QStringLiteral("Shapefiles (*.shp);;All Files (*)"));

	if (!strInputClippFilePath.isEmpty()) {
		// ���زü�ͼ��
		clippingLayer = new QgsVectorLayer(strInputClippFilePath, "Clipping Layer", "ogr");
		if (!clippingLayer->isValid()) {
			QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("���ؼ���ʸ����ʧ�ܣ�"));
			delete clippingLayer;
			clippingLayer = nullptr;
			return;
		}

		ui.clipLineEdit->setText(strInputClippFilePath);

		// ��ͼ����ӵ� QgsProject ��
		QgsProject::instance()->addMapLayer(clippingLayer);

		// ��ӵ� mapCanvas ��ͼ���б���
		QList<QgsMapLayer*> layers = mapCanvas->layers();
		layers.append(clippingLayer);
	}
}

bool VectorClippingFunction::clippingVector(const QString& outputPath)
{
	// ��ȡ����ͼ��� CRS
	QgsCoordinateReferenceSystem inputCrs = inputLayer->crs();

	// ����һ����ʱͼ�㣬���ڴ洢�ü���������� CRS Ϊ����ͼ��� CRS
	QString resultLayerCrs = QString("Polygon?crs=%1").arg(inputCrs.authid());
	QgsVectorLayer* resultLayer = new QgsVectorLayer(resultLayerCrs, "Clipped Layer", "memory");
	if (!resultLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("������ʱ�����ʧ��!"));
		return false;
	}

	// ��ȡ����ͼ����ֶ�
	QgsFields fields = inputLayer->fields();

	// �� QgsFields ת��Ϊ QList<QgsField>
	QList<QgsField> fieldList;
	for (int i = 0; i < fields.count(); ++i) {
		fieldList.append(fields.field(i));
	}

	// ����ֶε����ͼ��������ṩ������
	resultLayer->dataProvider()->addAttributes(fieldList);
	resultLayer->updateFields();

	// �������ü�ͼ���Ҫ��
	QgsFeatureIterator inputFeatures = inputLayer->getFeatures();
	QgsFeature inputFeature;

	while (inputFeatures.nextFeature(inputFeature)) {
		QgsGeometry inputGeometry = inputFeature.geometry();

		// �����ü�ͼ���Ҫ��
		QgsFeatureIterator clipFeatures = clippingLayer->getFeatures();
		QgsFeature clipFeature;

		while (clipFeatures.nextFeature(clipFeature)) {
			QgsGeometry clipGeometry = clipFeature.geometry();

			// ���㽻��
			QgsGeometry clippedGeometry = inputGeometry.intersection(clipGeometry);

			if (!clippedGeometry.isEmpty()) {
				// �����ü����Ҫ��
				QgsFeature clippedFeature(fields);
				clippedFeature.setGeometry(clippedGeometry);

				// �����ֶ�����
				for (int i = 0; i < fields.count(); ++i) {
					clippedFeature.setAttribute(i, inputFeature.attribute(i));
				}

				// ��ӵ���ʱͼ��
				resultLayer->dataProvider()->addFeature(clippedFeature);
			}
		}
	}

	// ȷ����ʱͼ��� CRS ������ͼ��һ��
	resultLayer->setCrs(inputCrs);

	// ����ѡ��
	QgsVectorFileWriter::SaveVectorOptions saveOptions;
	saveOptions.driverName = "ESRI Shapefile";  // �����ʽ
	saveOptions.fileEncoding = "UTF-8";        // �ļ�����
	saveOptions.layerName = "ClippedLayer";    // ͼ������

	// ��ȡ����ת��������
	QgsCoordinateTransformContext transformContext = QgsProject::instance()->transformContext();

	// ʹ�� writeAsVectorFormatV3 ������
	QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormatV3(
		resultLayer,                  // ����ͼ��
		outputPath,                   // ����ļ�·��
		transformContext,             // ����ת��������
		saveOptions                   // ����ѡ��
	);

	// ��鱣����
	if (error != QgsVectorFileWriter::NoError) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("������в㵽ָ��·��ʧ�ܣ�"));
		delete resultLayer;
		return false;
	}

	// ɾ����ʱͼ�����ͷ��ڴ�
	delete resultLayer;

	return true;
}

bool VectorClippingFunction::clippingTempVector(const QString& outputPath)
{
	if (!inputLayer || !clippingLayer) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("�����ͼ��в㶼�����ڼ���֮ǰ���أ�"));
		return false;
	}

	// ��ȡ����ͼ�������ϵ
	QgsCoordinateReferenceSystem inputCrs = inputLayer->crs();

	// ����һ����ʱͼ�����ڴ洢�ü������������Ϊ����ͼ�������ϵ
	QString wktCrs = inputCrs.toWkt();
	QgsVectorLayer* resultLayer = new QgsVectorLayer(QString("Polygon?crs=%1").arg(wktCrs), "Clipped Layer", "memory");
	if (!resultLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("���������ʧ�ܣ�"));
		return false;
	}

	// ��ȡ����ͼ����ֶ�
	QgsFields fields = inputLayer->fields();
	resultLayer->dataProvider()->addAttributes(fields.toList());
	resultLayer->updateFields();

	// �������ü�ͼ���Ҫ��
	QgsFeatureIterator inputFeatures = inputLayer->getFeatures();
	QgsFeature inputFeature;

	while (inputFeatures.nextFeature(inputFeature)) {
		QgsGeometry inputGeometry = inputFeature.geometry();

		// �����ü�ͼ���Ҫ��
		QgsFeatureIterator clipFeatures = clippingLayer->getFeatures();
		QgsFeature clipFeature;

		while (clipFeatures.nextFeature(clipFeature)) {
			QgsGeometry clipGeometry = clipFeature.geometry();

			// ���㽻��
			QgsGeometry clippedGeometry = inputGeometry.intersection(clipGeometry);

			if (!clippedGeometry.isEmpty()) {
				// �����ü����Ҫ��
				QgsFeature clippedFeature(fields);
				clippedFeature.setGeometry(clippedGeometry);

				// �����ֶ�����
				for (int i = 0; i < fields.count(); ++i) {
					clippedFeature.setAttribute(i, inputFeature.attribute(i));
				}

				// ��ӵ����ͼ��
				resultLayer->dataProvider()->addFeature(clippedFeature);
			}
		}
	}

	// ������ͼ��
	QgsVectorFileWriter::SaveVectorOptions saveOptions;
	saveOptions.driverName = "ESRI Shapefile";
	saveOptions.fileEncoding = "UTF-8";

	// ʹ������ͼ�������ת��������
	QgsCoordinateTransformContext transformContext = QgsProject::instance()->transformContext();

	QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormatV3(
		resultLayer, outputPath, transformContext, saveOptions
	);

	if (error != QgsVectorFileWriter::NoError) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("�������ͼ��ʧ��!"));
		delete resultLayer;
		return false;
	}

	delete resultLayer;
	return true;
}

void VectorClippingFunction::drawClippingVector(const QString& geometryType)
{
	// ��ȡѡ�е�����
	int currentIndex = ui.inputVectorBox->currentIndex();

	if (currentIndex < 0 || currentIndex >= loadedLayers.size()) {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("ʸ��ͼ�㲻���ڣ�"));
		return;
	}
	if (strInputFilePath.isEmpty()) {
		// ���ڵ�ǰ������ȡѡ�е�ͼ��
		inputLayer = loadedLayers[currentIndex];
	}
	
	DrawGeometryTool::DrawShape shape;
	if (geometryType == QStringLiteral("����")) {
		shape = DrawGeometryTool::Rectangle;
	}
	else if (geometryType == QStringLiteral("Բ��")) {
		shape = DrawGeometryTool::Circle;
	}
	else if (geometryType == QStringLiteral("�����")) {
		shape = DrawGeometryTool::Polygon;
	}
	else {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("��Ч�ļ������ͣ�"));
		return;
	}

	auto* drawTool = new DrawGeometryTool(mapCanvas, shape);
	connect(drawTool, &DrawGeometryTool::geometryDrawn, this, [this](const QgsGeometry& geometry) {
		if (!inputLayer) {
			QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("û�м����������ȷ������ο�ϵ��"));
			return;
		}

		// ��ȡ����ͼ�������ϵ
		const QgsCoordinateReferenceSystem crs = inputLayer->crs();

		// ������ʱͼ�㲢����������ͼ����ͬ������ϵ
		QString wktCrs = crs.toWkt();
		QgsVectorLayer* tempLayer = new QgsVectorLayer(QString("Polygon?crs=%1").arg(wktCrs), "Clipping Layer", "memory");
		if (!tempLayer->isValid()) {
			QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("������ʱ��ʧ�ܣ�"));
			return;
		}

		// ��Ӽ��ε�ͼ��
		QgsFeature feature;
		feature.setGeometry(geometry);
		tempLayer->dataProvider()->addFeature(feature);
		tempLayer->updateExtents();
		tempLayer->commitChanges();

		// ����ʱͼ����ӵ���Ŀ��
		QgsProject::instance()->addMapLayer(tempLayer);

		// ���òü�ͼ��
		clippingLayer = tempLayer;

		// ��ȡ��ǰͼ���б�����ͼ�����ڶ���
		QList<QgsMapLayer*> layers = mapCanvas->layers();
		layers.append(tempLayer);  // �����ͼ��
		layers.move(layers.size() - 1, 0);  // ����ͼ���ƶ�������
		mapCanvas->setLayers(layers);  // ����ͼ��˳��

		// ���µ�ͼ��ʾ��Χ
		mapCanvas->refresh();

		});

	mapCanvas->setMapTool(drawTool);
}

void VectorClippingFunction::onCreateClippingVector()
{
	QStringList options = { QStringLiteral("����"), QStringLiteral("Բ��"), QStringLiteral("�����") };
	bool ok;
	QString shapeType = QInputDialog::getItem(this, QStringLiteral("����һ��ʸ����Ҫ��"), QStringLiteral("ѡ����Ҫ������:"), options, 0, false, &ok);

	if (ok && !shapeType.isEmpty()) {
		drawClippingVector(shapeType);
	}
}

void VectorClippingFunction::showMapLayerList() {
	// ����һ����׼��ģ��
	QStandardItemModel* model = new QStandardItemModel(this);

	// ��ȡ��Ŀ�е�����ͼ��
	const auto layers = QgsProject::instance()->mapLayers();

	// ��������ͼ�㲢���ļ�����ӵ�ģ����
	for (const auto& layerPair : layers) {
		QgsMapLayer* layer = layerPair; // ��ȡͼ�����

		QString displayName;

		if (auto vectorLayer = qobject_cast<QgsVectorLayer*>(layer)) {
			// ��ȡ�ļ���
			QFileInfo fileInfo(vectorLayer->source());
			displayName = fileInfo.fileName(); // ��ȡ�ļ�����������չ����
		}
		else {
			// �������ʸ��ͼ�㣬ֱ��ʹ��ͼ������
			displayName = layer->name();
		}

		QStandardItem* item = new QStandardItem(displayName);
		model->appendRow(item);
	}
}

void VectorClippingFunction::showMapLayerBox() {
	// ���������������Ա����ظ�
	ui.inputVectorBox->clear();
	ui.clippiingVectorBox->clear();

	// ����Ѽ���ͼ���б�
	loadedLayers.clear();

	// ��ȡ��Ŀ�е�����ͼ��
	const auto layers = QgsProject::instance()->mapLayers();

	for (const auto& layerPair : layers) {
		QgsMapLayer* layer = layerPair; // ��ȡͼ�����
		if (layer->type() == Qgis::LayerType::Vector) {
			// ת��Ϊ QgsVectorLayer
			if (auto vectorLayer = qobject_cast<QgsVectorLayer*>(layer)) {
				// ��ȡͼ���ļ�·��
				QString sourcePath = vectorLayer->source();

				// ��ȡ�ļ���
				QFileInfo fileInfo(sourcePath);
				QString fileName = fileInfo.fileName(); // ��ȡ�ļ���������չ����

				// ����ļ�����������
				ui.inputVectorBox->addItem(fileName);
				ui.clippiingVectorBox->addItem(fileName);

				// ��ӵ��Ѽ���ͼ���б�
				loadedLayers.append(vectorLayer);

				QgsProject::instance()->addMapLayer(layer);
				mapCanvas->refresh();
			}
		}
	}
}




