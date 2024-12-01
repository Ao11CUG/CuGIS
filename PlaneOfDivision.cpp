#include "PlaneOfDivision.h"

PlaneOfDivision::PlaneOfDivision(QgsMapCanvas* m_mapCanvas, QWidget* parent)
	: QMainWindow(parent), m_mapCanvas(m_mapCanvas)
{
	ui.setupUi(this);

	setStyle();

	setWindowIcon(QIcon(":/MainInterface/res/surface.png"));
	setWindowTitle(QStringLiteral("�ָ���"));

	mapCanvas = new QgsMapCanvas();
	mapCanvas->setCanvasColor(Qt::white);  // ���ñ�����ɫ
	mapCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);  // ���ô�С��������Ӧ��������

	// �� QgsMapCanvas ���� QScrollArea ��
	ui.showSurfaceArea->setWidget(mapCanvas);

	// ȷ����Ŀ�Ѽ���
	QgsProject::instance();

	updateView();
	showMapLayerBox();

	// �����������ķ�Χ����ź�
	connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &PlaneOfDivision::updateView);
	connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &PlaneOfDivision::showMapLayerBox);

	// ���Ӱ�ť�¼�
	connect(ui.inputVectorData, &QPushButton::clicked, this, &PlaneOfDivision::onInputVectorDataClicked);
	connect(ui.createDivision, &QPushButton::clicked, this, &PlaneOfDivision::onCreateDivisionClicked);
	connect(ui.actionDivision, &QPushButton::clicked, this, &PlaneOfDivision::onActionDivisionClicked);
}

PlaneOfDivision::~PlaneOfDivision()
{}

void PlaneOfDivision::setStyle() {
	style.setConfirmButton(ui.actionDivision);

	style.setConfirmButton(ui.createDivision);

	style.setCommonButton(ui.inputVectorData);

	style.setLineEdit(ui.lineEdit);

	style.setComboBox(ui.comboBox);
}

void PlaneOfDivision::updateView() {
	mapCanvas->setExtent(m_mapCanvas->extent());
	mapCanvas->setLayers(m_mapCanvas->layers());
}

void PlaneOfDivision::onInputVectorDataClicked()
{
	// ���ļ��Ի���ѡ��ʸ���ļ�
	strInputFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("ѡ��ʸ����Ҫ��"), "", QStringLiteral("Shapefiles (*.shp);;All Files (*)"));
	if (!strInputFilePath.isEmpty()) {
		showVectorData(strInputFilePath);
	}
}

void PlaneOfDivision::showVectorData(const QString& filePath)
{
	// ����ʸ��ͼ�㲢�����ļ�
	inputLayer = new QgsVectorLayer(filePath, QFileInfo(filePath).baseName(), "ogr");
	if (!inputLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("����ʧ��"), QStringLiteral("�޷�����ʸ��ͼ�㣡"));
		return;
	}

	// ��ͼ����ӵ� QgsProject ��
	QgsProject::instance()->addMapLayer(inputLayer);

	// ��ȡ��ǰ mapCanvas ͼ���б�
	QList<QgsMapLayer*> layers = mapCanvas->layers();
	layers.append(inputLayer);  // ����ͼ����ӵ�ͼ���б���
	mapCanvas->setLayers(layers);  // ����ͼ���б�

	// �Զ�������ͼ��Χ����Ӧ��ͼ��
	mapCanvas->zoomToFullExtent();

	// ����ͼ���ƶ�������
	layers.move(layers.size() - 1, 0);
	mapCanvas->setLayers(layers);  // ����ͼ��˳��

	// ˢ�� mapCanvas
	mapCanvas->refresh();
}

void PlaneOfDivision::onFeatureIdentified(const QgsFeature& feature)
{
	if (!isSelecting) return;

	if (!inputLayer || !inputLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("ʸ��ͼ����Ч��"));
		return;
	}

	// ����Ƿ�ѡ����Ч����Ҫ��
	if (feature.geometry().wkbType() != Qgis::WkbType::Polygon &&
		feature.geometry().wkbType() != Qgis::WkbType::MultiPolygon) {
		QMessageBox::warning(this, QStringLiteral("����"), QStringLiteral("��ѡ��һ����Ҫ�أ�"));
		return;
	}

	// ��ѡ�е�Ҫ�ر��沢����ѡ��ģʽ
	selectedFeatures.append(feature);
	isSelecting = false;

	// ��ʾ�û�
	QMessageBox::information(this, QStringLiteral("��ʾ"), QStringLiteral("��ѡ����Ҫ�أ����������һ������Ρ�"));

	// ���û��ƶ���ε��߼�
	showDivisionData();
}

void PlaneOfDivision::onCreateDivisionClicked()
{
	if (!isSelecting) {
		isSelecting = true;  // ����ѡ��ģʽ
		selectedFeatures.clear();  // �����ѡ���Ҫ��
	}

	// ���ҵ�ǰ��ͼ������ʸ��ͼ��
	inputLayer = nullptr;
	for (QgsMapLayer* mapLayer : mapCanvas->layers()) {
		if (mapLayer->type() == Qgis::LayerType::Vector) {
			inputLayer = qobject_cast<QgsVectorLayer*>(mapLayer);
			break;
		}
	}

	if (!inputLayer || !inputLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("δ�ҵ���Ч��ʸ��ͼ�㣡"));
		return;
	}

	// ��ʼ�� identifyTool �������źŲ�
	if (!identifyTool) {
		identifyTool = new QgsMapToolIdentifyFeature(mapCanvas, inputLayer);
		connect(identifyTool, SIGNAL(featureIdentified(const QgsFeature&)), this, SLOT(onFeatureIdentified(const QgsFeature&)));
	}

	// ����ʶ�𹤾�Ϊ��ǰ��ͼ����
	mapCanvas->setMapTool(identifyTool);

	// ��ʾ�û�ѡ�����
	QMessageBox::information(this, QStringLiteral("��ʾ"), QStringLiteral("��ѡ��һ����Ҫ�أ���ɺ󽫼�����һ��������"));
}

void PlaneOfDivision::highlightSelectedFeatures(const QList<QgsFeature>& features)
{
	if (!rubberBand) {
		// ��ʼ�� rubberBand Ϊ����θ���
		rubberBand = new QgsRubberBand(mapCanvas, Qgis::GeometryType::Polygon);
		rubberBand->setColor(Qt::yellow); // ���ø�����ɫ
		rubberBand->setWidth(200);       // �����߿�
	}

	rubberBand->reset(Qgis::GeometryType::Polygon);  // ���ø�������

	if (!inputLayer) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("��Ч������ͼ�㣡"));
		return;
	}

	QgsCoordinateTransform transform(inputLayer->crs(), mapCanvas->mapSettings().destinationCrs(), QgsProject::instance());

	for (const QgsFeature& feature : features) {
		if (!feature.isValid()) {
			continue;
		}

		QgsGeometry geom = feature.geometry();
		if (geom.isNull() || !geom.isGeosValid()) {
			QMessageBox::warning(this, QStringLiteral("����"), QStringLiteral("������Ч���޷�������"));
			continue;
		}

		// ��ȡ���εĵ�һ���ͣ����罫MultiPolygonת��ΪPolygon��
		Qgis::WkbType singleType = QgsWkbTypes::singleType(geom.wkbType());

		// ��鼸���Ƿ���֧�ֵ�����
		if (singleType == Qgis::WkbType::Polygon || singleType == Qgis::WkbType::LineString) {
			// ת�����ε�������Ŀ������ο�ϵͳ
			QgsGeometry transformedGeom = geom;
			transformedGeom.transform(transform);
			rubberBand->addGeometry(transformedGeom, nullptr); // ��Ӽ���
		}
		else {
			QMessageBox::warning(this, QStringLiteral("����"), QStringLiteral("�������Ͳ�֧�ָ�����ʾ��"));
		}
	}

	rubberBand->show(); // ȷ��������ʾ
	mapCanvas->refresh(); // ˢ�»���
}

void PlaneOfDivision::onActionDivisionClicked()
{
	// ���� actionDivision �������зָ����
	actionDivision();
}

void PlaneOfDivision::actionDivision()
{
	// ��ȡѡ�е�����
	int currentIndex = ui.comboBox->currentIndex();

	if (currentIndex < 0 || currentIndex >= loadedLayers.size()) {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("ʸ��ͼ�㲻���ڣ�"));
		return;
	}
	if (strInputFilePath.isEmpty()) {
		// ���ڵ�ǰ������ȡѡ�е�ͼ��
		inputLayer = loadedLayers[currentIndex];
	}
	if (!inputLayer || !DivisionLayer) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("����ͼ���ָ�ͼ��δ���أ�"));
		return;
	}

	if (!inputLayer->isValid() || !DivisionLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("����ͼ���ָ�ͼ����Ч��"));
		return;
	}

	// ȷ������ͼ�㶼����ͼ��
	if (inputLayer->geometryType() != Qgis::GeometryType::Polygon || DivisionLayer->geometryType() != Qgis::GeometryType::Polygon) {
		QMessageBox::critical(this, QStringLiteral("����"), QStringLiteral("����ͼ��ͷָ�ͼ�㶼��������ͼ�㣡"));
		return;
	}

	QgsFeatureList newFeatures; // ���ڴ洢�ָ�����Ҫ��
	QgsFeatureIterator inputIterator = inputLayer->getFeatures(); // ��ȡĿ��ͼ���Ҫ�ص�����

	QgsFeature inputFeature;
	while (inputIterator.nextFeature(inputFeature)) { // ���� inputLayer ��ÿ��Ҫ��
		QgsGeometry inputGeometry = inputFeature.geometry();
		if (inputGeometry.isNull() || !inputGeometry.isGeosValid()) continue;

		QgsFeatureIterator divisionIterator = DivisionLayer->getFeatures(); // ��ȡ�ָ�ͼ���Ҫ�ص�����
		QgsFeature divisionFeature;
		QgsGeometry currentGeometry = inputGeometry;

		// �����ָ�ͼ���е�Ҫ��
		while (divisionIterator.nextFeature(divisionFeature)) {
			QgsGeometry divisionGeometry = divisionFeature.geometry();
			if (divisionGeometry.isNull() || !divisionGeometry.isGeosValid()) continue;

			// ����Ƿ��ཻ
			if (!currentGeometry.intersects(divisionGeometry)) {
				continue;
			}

			// �����ཻ����
			QgsGeometry intersectedGeometry = currentGeometry.intersection(divisionGeometry);
			if (!intersectedGeometry.isNull() && intersectedGeometry.isGeosValid()) {
				// �����µ�Ҫ�ر����ཻ����
				QgsFeature newFeature(inputFeature.fields());
				newFeature.setGeometry(intersectedGeometry);
				newFeatures.append(newFeature);
			}

			// ���µ�ǰ����Ϊδ�ཻ����
			currentGeometry = currentGeometry.difference(divisionGeometry);
		}

		// �������δ�ཻ���֣����µ�ԭͼ��
		if (!currentGeometry.isNull() && currentGeometry.isGeosValid()) {
			inputFeature.setGeometry(currentGeometry);
			inputLayer->updateFeature(inputFeature);
		}
	}

	// ����Ҫ����ӵ�����ͼ��
	if (!newFeatures.isEmpty()) {
		inputLayer->dataProvider()->addFeatures(newFeatures);
	}

	// ˢ������ͼ��
	inputLayer->triggerRepaint();

	QMessageBox::information(this, QStringLiteral("���"), QStringLiteral("�ָ������ɣ�"));

}

void PlaneOfDivision::showDivisionData() {

	QStringList options = { QStringLiteral("����"), QStringLiteral("Բ��"), QStringLiteral("�����") };
	bool ok;
	QString shapeType = QInputDialog::getItem(this, QStringLiteral("����һ��ʸ����Ҫ��"), QStringLiteral("ѡ����Ҫ������:"), options, 0, false, &ok);

	if (ok && !shapeType.isEmpty()) {
		createDivisionPolygon(shapeType);
	}

}

void PlaneOfDivision::createDivisionPolygon(const QString& geometryType) {
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

		// ���÷ָ�ͼ��
		DivisionLayer = tempLayer;

		// ��ȡ��ǰͼ���б�����ͼ�����ڶ���
		QList<QgsMapLayer*> layers = mapCanvas->layers();
		layers.append(tempLayer);  // �����ͼ��
		mapCanvas->setLayers(layers);  // ����ͼ��˳��

		// ���µ�ͼ��ʾ��Χ
		mapCanvas->refresh();

		});

	mapCanvas->setMapTool(drawTool);


}

void PlaneOfDivision::showMapLayerBox() {
	// ���������������Ա����ظ�
	ui.comboBox->clear();

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
				ui.comboBox->addItem(fileName);

				// ��ӵ��Ѽ���ͼ���б�
				loadedLayers.append(vectorLayer);

				QgsProject::instance()->addMapLayer(layer);
				mapCanvas->refresh();
			}
		}
	}
}
