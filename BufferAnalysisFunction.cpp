#include "BufferAnalysisFunction.h"
#include "qpushbutton.h"
#include "qgsmapcanvas.h"

BufferAnalysisFunction::BufferAnalysisFunction(QgsMapCanvas* m_mapCanvas, QWidget* parent)
	: QMainWindow(parent), m_mapCanvas(m_mapCanvas)
{
	ui.setupUi(this);

	setWindowIcon(QIcon(":/MainInterface/res/bufferAnalysis.png"));
	setWindowTitle(QStringLiteral("矢量缓冲区分析"));

	setStyle();

	// 连接信号和槽
	connect(ui.chooseVectorFile, &QPushButton::clicked, this, &BufferAnalysisFunction::onChooseVectorFileClicked);
	connect(ui.chooseSaveFile, &QPushButton::clicked, this, &BufferAnalysisFunction::onChooseSaveFileClicked);
	connect(ui.ActionBufferAnalysis, &QPushButton::clicked, this, &BufferAnalysisFunction::onActionBufferAnalysisClicked);

	connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &BufferAnalysisFunction::updateBox);
}

BufferAnalysisFunction::~BufferAnalysisFunction()
{}

void BufferAnalysisFunction::updateBox() {
	// 清空组合框
	ui.fileComboBox->clear();
	m_vectorLayers.clear(); // 确保清空先前的图层

	// 获取项目中的所有图层
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
	// 打开文件对话框选择矢量文件
	strInputFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择矢量文件"), "", "Shapefiles (*.shp)");

	ui.inputLineEdit->setText(strInputFilePath);
}

void BufferAnalysisFunction::onChooseSaveFileClicked()
{
	// 打开文件对话框选择保存路径
	strOutputFilePath = QFileDialog::getSaveFileName(this, QStringLiteral("选择保存路径"), "", "Shapefiles (*.shp)");
	if (strOutputFilePath.isEmpty()) {
		return;
	}

	ui.outputLineEdit->setText(strOutputFilePath);
}

void BufferAnalysisFunction::onActionBufferAnalysisClicked()
{
	if (strOutputFilePath.isEmpty()) {
		QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("请设置结果保存路径！"));
		return;
	}

	bool bBufferRadius;
	double dBufferRadius = ui.BufferRadius->text().toDouble(&bBufferRadius);
	if (!bBufferRadius || dBufferRadius <= 0) {
		QMessageBox::warning(this, tr("Error"), QStringLiteral("请输入有效的缓冲区半径！"));
		return;
	}

	performBufferAnalysis(strInputFilePath, strOutputFilePath, dBufferRadius);

	QFileInfo fi(strOutputFilePath);
	if (!fi.exists()) { return; }
	QString layerBaseName = fi.baseName(); // 图层名称

	QgsVectorLayer* resultLayer = new QgsVectorLayer(strOutputFilePath, layerBaseName);

	QgsProject::instance()->addMapLayer(resultLayer);
}

void BufferAnalysisFunction::performBufferAnalysis(const QString& strInputFilePath, const QString& strOutputFilePath, double dBufferRadius)
{
	// 获取选中的索引
	int currentIndex = ui.fileComboBox->currentIndex();

	if (currentIndex < 0 || currentIndex >= m_vectorLayers.size()) {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("矢量图层不存在！"));
		return;
	}

	// 基于当前索引获取选中的图层
	m_layer = m_vectorLayers[currentIndex];

	if (!strInputFilePath.isEmpty()) {
		// 加载矢量图层
		QgsVectorLayer* inputLayer = new QgsVectorLayer(strInputFilePath, "InputLayer", "ogr");
		if (!inputLayer || !inputLayer->isValid()) {
			QMessageBox::critical(this, tr("Error"), QStringLiteral("加载输入图层失败！"));
			delete inputLayer;
			return;
		}
	}

	QgsVectorLayer* inputLayer = m_layer;

	// 创建输出图层
	QString crsWkt = inputLayer->crs().toWkt(); // 获取 CRS
	QgsFields fields = inputLayer->fields();    // 获取原始字段

	QgsVectorLayer* bufferLayer = new QgsVectorLayer(
		QString("Polygon?crs=%1").arg(crsWkt), "BufferLayer", "memory");
	if (!bufferLayer || !bufferLayer->isValid()) {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("生成缓冲区图层失败！"));
		delete inputLayer;
		return;
	}

	bufferLayer->dataProvider()->addAttributes(fields.toList());
	bufferLayer->updateFields();

	// 执行缓冲区分析
	QgsFeatureIterator featureIterator = inputLayer->getFeatures();
	QgsFeature inputFeature, bufferFeature;
	QgsGeometry bufferGeometry;
	QList<QgsFeature> bufferFeatures;

	while (featureIterator.nextFeature(inputFeature)) {
		QgsGeometry inputGeometry = inputFeature.geometry();
		if (inputGeometry.isEmpty()) continue;

		bufferGeometry = inputGeometry.buffer(dBufferRadius, 30); // 缓冲区计算
		if (bufferGeometry.isEmpty()) continue;

		bufferFeature.setGeometry(bufferGeometry);
		bufferFeature.setAttributes(inputFeature.attributes());
		bufferFeatures.append(bufferFeature);
	}

	bufferLayer->dataProvider()->addFeatures(bufferFeatures);

	// 使用 QgsVectorFileWriter::writeAsVectorFormatV2 保存缓冲区结果
	QgsVectorFileWriter::SaveVectorOptions saveOptions;
	saveOptions.driverName = "ESRI Shapefile";  // 保存为 ESRI Shapefile 格式
	saveOptions.fileEncoding = "UTF-8";         // 文件编码
	saveOptions.layerName = "BufferAnalysisResult";

	// 修正：确保 transformContext 类型完整，包含必要的头文件
	QgsCoordinateTransformContext transformContext = QgsProject::instance()->transformContext();

	QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormatV3(
		bufferLayer,                          // 输入图层
		strOutputFilePath,                           // 输出文件路径
		transformContext,                     // 坐标转换上下文
		saveOptions                           // 保存选项
	);

	// 输出保存结果
	if (error == QgsVectorFileWriter::NoError) {
		QgsVectorLayer checkLayer(strOutputFilePath, "ExportedLayer", "ogr");
	}
	else {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("保存结果至指定路径失败！"));
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


