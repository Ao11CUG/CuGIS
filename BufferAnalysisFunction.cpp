#include "BufferAnalysisFunction.h"
#include "qpushbutton.h"
#include "qgsmapcanvas.h"

BufferAnalysisFunction::BufferAnalysisFunction(QgsMapCanvas* m_mapCanvas, QWidget* parent)
	: QMainWindow(parent), m_mapCanvas(m_mapCanvas)
{
	ui.setupUi(this);

	setWindowIcon(QIcon(":/MainInterface/res/bufferAnalysis.png"));
	setWindowTitle(QStringLiteral("ʸ������������"));

	setStyle();

	// �����źźͲ�
	connect(ui.chooseVectorFile, &QPushButton::clicked, this, &BufferAnalysisFunction::onChooseVectorFileClicked);
	connect(ui.chooseSaveFile, &QPushButton::clicked, this, &BufferAnalysisFunction::onChooseSaveFileClicked);
	connect(ui.ActionBufferAnalysis, &QPushButton::clicked, this, &BufferAnalysisFunction::onActionBufferAnalysisClicked);

	connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &BufferAnalysisFunction::updateBox);
}

BufferAnalysisFunction::~BufferAnalysisFunction()
{}

void BufferAnalysisFunction::updateBox() {
	// �����Ͽ�
	ui.fileComboBox->clear();
	m_vectorLayers.clear(); // ȷ�������ǰ��ͼ��

	// ��ȡ��Ŀ�е�����ͼ��
	const auto layers = QgsProject::instance()->mapLayers();
	for (const auto& layer : layers) {
		if (layer->type() == Qgis::LayerType::Vector) {
			ui.fileComboBox->addItem(layer->name());
			m_vectorLayers.append(qobject_cast<QgsVectorLayer*>(layer));
		}
	}
}

void BufferAnalysisFunction::onChooseVectorFileClicked()
{
	// ���ļ��Ի���ѡ��ʸ���ļ�
	strInputFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("ѡ��ʸ���ļ�"), "", "Shapefiles (*.shp)");

	ui.inputLineEdit->setText(strInputFilePath);
}

void BufferAnalysisFunction::onChooseSaveFileClicked()
{
	// ���ļ��Ի���ѡ�񱣴�·��
	strOutputFilePath = QFileDialog::getSaveFileName(this, QStringLiteral("ѡ�񱣴�·��"), "", "Shapefiles (*.shp)");
	if (strOutputFilePath.isEmpty()) {
		return;
	}

	ui.outputLineEdit->setText(strOutputFilePath);
}

void BufferAnalysisFunction::onActionBufferAnalysisClicked()
{
	if (strOutputFilePath.isEmpty()) {
		QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("�����ý������·����"));
		return;
	}

	bool bBufferRadius;
	double dBufferRadius = ui.BufferRadius->text().toDouble(&bBufferRadius);
	if (!bBufferRadius || dBufferRadius <= 0) {
		QMessageBox::warning(this, tr("Error"), QStringLiteral("��������Ч�Ļ������뾶��"));
		return;
	}

	performBufferAnalysis(strInputFilePath, strOutputFilePath, dBufferRadius);

	QFileInfo fi(strOutputFilePath);
	if (!fi.exists()) { return; }
	QString layerBaseName = fi.baseName(); // ͼ������

	QgsVectorLayer* resultLayer = new QgsVectorLayer(strOutputFilePath, layerBaseName);

	QgsProject::instance()->addMapLayer(resultLayer);
}

void BufferAnalysisFunction::performBufferAnalysis(const QString& strInputFilePath, const QString& strOutputFilePath, double dBufferRadius)
{
	// ��ȡѡ�е�����
	int currentIndex = ui.fileComboBox->currentIndex();

	if (currentIndex < 0 || currentIndex >= m_vectorLayers.size()) {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("ʸ��ͼ�㲻���ڣ�"));
		return;
	}

	// ���ڵ�ǰ������ȡѡ�е�ͼ��
	m_layer = m_vectorLayers[currentIndex];

	if (!strInputFilePath.isEmpty()) {
		// ����ʸ��ͼ��
		QgsVectorLayer* inputLayer = new QgsVectorLayer(strInputFilePath, "InputLayer", "ogr");
		if (!inputLayer || !inputLayer->isValid()) {
			QMessageBox::critical(this, tr("Error"), QStringLiteral("��������ͼ��ʧ�ܣ�"));
			delete inputLayer;
			return;
		}
	}

	QgsVectorLayer* inputLayer = m_layer;

	// �������ͼ��
	QString crsWkt = inputLayer->crs().toWkt(); // ��ȡ CRS
	QgsFields fields = inputLayer->fields();    // ��ȡԭʼ�ֶ�

	QgsVectorLayer* bufferLayer = new QgsVectorLayer(
		QString("Polygon?crs=%1").arg(crsWkt), "BufferLayer", "memory");
	if (!bufferLayer || !bufferLayer->isValid()) {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("���ɻ�����ͼ��ʧ�ܣ�"));
		delete inputLayer;
		return;
	}

	bufferLayer->dataProvider()->addAttributes(fields.toList());
	bufferLayer->updateFields();

	// ִ�л���������
	QgsFeatureIterator featureIterator = inputLayer->getFeatures();
	QgsFeature inputFeature, bufferFeature;
	QgsGeometry bufferGeometry;
	QList<QgsFeature> bufferFeatures;

	while (featureIterator.nextFeature(inputFeature)) {
		QgsGeometry inputGeometry = inputFeature.geometry();
		if (inputGeometry.isEmpty()) continue;

		bufferGeometry = inputGeometry.buffer(dBufferRadius, 30); // ����������
		if (bufferGeometry.isEmpty()) continue;

		bufferFeature.setGeometry(bufferGeometry);
		bufferFeature.setAttributes(inputFeature.attributes());
		bufferFeatures.append(bufferFeature);
	}

	bufferLayer->dataProvider()->addFeatures(bufferFeatures);

	// ʹ�� QgsVectorFileWriter::writeAsVectorFormatV2 ���滺�������
	QgsVectorFileWriter::SaveVectorOptions saveOptions;
	saveOptions.driverName = "ESRI Shapefile";  // ����Ϊ ESRI Shapefile ��ʽ
	saveOptions.fileEncoding = "UTF-8";         // �ļ�����
	saveOptions.layerName = "BufferAnalysisResult";

	// ������ȷ�� transformContext ����������������Ҫ��ͷ�ļ�
	QgsCoordinateTransformContext transformContext = QgsProject::instance()->transformContext();

	QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormatV3(
		bufferLayer,                          // ����ͼ��
		strOutputFilePath,                           // ����ļ�·��
		transformContext,                     // ����ת��������
		saveOptions                           // ����ѡ��
	);

	// ���������
	if (error == QgsVectorFileWriter::NoError) {
		QgsVectorLayer checkLayer(strOutputFilePath, "ExportedLayer", "ogr");
	}
	else {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("��������ָ��·��ʧ�ܣ�"));
	}
}

void BufferAnalysisFunction::setStyle() {
	style.setConfirmButton(ui.ActionBufferAnalysis);

	style.setCommonButton(ui.chooseVectorFile);

	style.setCommonButton(ui.chooseSaveFile);

	style.setLineEdit(ui.inputLineEdit);

	style.setLineEdit(ui.outputLineEdit);

	style.setLineEdit(ui.BufferRadius);

	style.setComboBox(ui.fileComboBox);
}


