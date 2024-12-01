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
	setWindowTitle(QStringLiteral("矢量裁剪分析"));

	// 确保项目已加载
	QgsProject::instance();
	
	setStyle();

	// 创建 QgsmapCanvas 并将其嵌入到 QScrollArea 中
	mapCanvas = new QgsMapCanvas();
	mapCanvas->setCanvasColor(Qt::white);  // 设置背景颜色

	// 将 QgsmapCanvas 放入 QScrollArea 中
	ui.viewArea->setWidget(mapCanvas);  // 将 mapCanvas 作为 QScrollArea 的内容

	// 连接信号与槽
	connect(ui.inputVectorData, &QPushButton::clicked, this, &VectorClippingFunction::onInputVectorData);
	connect(ui.inputClippData, &QPushButton::clicked, this, &VectorClippingFunction::onInputClippData);
	connect(ui.createClippingVector, &QPushButton::clicked, this, &VectorClippingFunction::onCreateClippingVector);
	connect(ui.actionClipping, &QPushButton::clicked, this, &VectorClippingFunction::onActionClipping);

	updateView();

	// 连接主画布的范围变更信号
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
	// 创建 QgsVectorLayer 并加载矢量数据
	QgsVectorLayer* vectorLayer = new QgsVectorLayer(filePath, QStringLiteral("输入矢量面要素"), "ogr");

	if (!vectorLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("加载矢量层失败！"));
		return;
	}

	// 将新图层添加到 QgsProject 中
	QgsProject::instance()->addMapLayer(vectorLayer);

	// 将图层添加到已加载图层列表中
	loadedLayers.append(vectorLayer);  // 添加到 QList 中

	// 如果 mapCanvas 没有设置图层，初始化图层
	if (mapCanvas->layers().isEmpty()) {
		mapCanvas->setLayers({ vectorLayer });
	}
	else {
		// 如果已有图层，叠加新图层
		QList<QgsMapLayer*> layers = mapCanvas->layers();
		layers.append(vectorLayer);  // 添加新图层
		mapCanvas->setLayers(layers);  // 更新图层列表
	}

	// 自动调整地图显示范围以适应所有要素
	mapCanvas->zoomToFullExtent();

	// 刷新地图
	mapCanvas->refresh();
}

void VectorClippingFunction::onActionClipping()
{
	// 获取选中的索引
	int currentIndex = ui.inputVectorBox->currentIndex();

	if (currentIndex < 0 || currentIndex >= loadedLayers.size()) {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("矢量图层不存在！"));
		return;
	}
	if (strInputFilePath.isEmpty()) {
		// 基于当前索引获取选中的图层
		inputLayer = loadedLayers[currentIndex];
	}

	// 获取选中的索引
	int currentClipIndex = ui.clippiingVectorBox->currentIndex();

	if (currentClipIndex < 0 || currentClipIndex >= loadedLayers.size()) { 
		QMessageBox::critical(this, tr("Error"), QStringLiteral("矢量图层不存在！"));
		return;
	}
	if (strInputClippFilePath.isEmpty() && !clippingLayer) {
		// 基于当前索引获取选中的图层
		clippingLayer = loadedLayers[currentIndex];
	}
	
	// 确保被裁剪的图层和裁剪图层已加载
	if (!inputLayer || !clippingLayer) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("输入层和剪切层都必须在剪切之前加载!"));
		return;
	}

	// 打开文件对话框让用户选择保存路径
	strOutputFilePath = QFileDialog::getSaveFileName(this, QStringLiteral("保存裁剪图层至"), "", QStringLiteral("Shapefiles (*.shp)"));

	if (strOutputFilePath.isEmpty()) {
		return; // 如果用户没有选择路径，直接返回
	}

	// 调用裁剪函数，将结果保存到指定路径
	if (!clippingVector(strOutputFilePath)) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("保存剪切图层到指定路径失败！"));
	}
	else {
		QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("被剪切的图层已成功保存为 %1").arg(strOutputFilePath));
	}
	// 加载裁剪结果到 canvas
	QgsVectorLayer* clippedLayer = new QgsVectorLayer(strOutputFilePath, "Clipped Result", "ogr");
	if (!clippedLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("加载剪切层失败！"));
		delete clippedLayer;
		return;
	}

	// 将裁剪结果添加到 QgsProject 中
	QgsProject::instance()->addMapLayer(clippedLayer);

	// 更新地图范围以适应裁剪结果
	mapCanvas->setExtent(clippedLayer->extent());
	mapCanvas->refresh();
}

void VectorClippingFunction::onInputVectorData()
{
	// 打开文件对话框选择矢量文件
	strInputFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择矢量面要素"), "", QStringLiteral("Shapefiles (*.shp);;All Files (*)"));

	if (!strInputFilePath.isEmpty()) {
		// 加载被裁剪图层
		inputLayer = new QgsVectorLayer(strInputFilePath, "Input Layer", "ogr");
		if (!inputLayer->isValid()) {
			QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("加载输入矢量层失败！"));
			delete inputLayer;
			inputLayer = nullptr;
			return;
		}

		ui.inputLineEdit->setText(strInputFilePath);

		// 将图层添加到 QgsProject 中
		QgsProject::instance()->addMapLayer(inputLayer);

		// 添加到 mapCanvas 的图层列表中
		QList<QgsMapLayer*> layers = mapCanvas->layers();
		layers.append(inputLayer);
	}
}

void VectorClippingFunction::onInputClippData()
{
	// 打开文件对话框选择裁剪矢量文件
	strInputClippFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择裁剪面要素："), "", QStringLiteral("Shapefiles (*.shp);;All Files (*)"));

	if (!strInputClippFilePath.isEmpty()) {
		// 加载裁剪图层
		clippingLayer = new QgsVectorLayer(strInputClippFilePath, "Clipping Layer", "ogr");
		if (!clippingLayer->isValid()) {
			QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("加载剪切矢量层失败！"));
			delete clippingLayer;
			clippingLayer = nullptr;
			return;
		}

		ui.clipLineEdit->setText(strInputClippFilePath);

		// 将图层添加到 QgsProject 中
		QgsProject::instance()->addMapLayer(clippingLayer);

		// 添加到 mapCanvas 的图层列表中
		QList<QgsMapLayer*> layers = mapCanvas->layers();
		layers.append(clippingLayer);
	}
}

bool VectorClippingFunction::clippingVector(const QString& outputPath)
{
	// 获取输入图层的 CRS
	QgsCoordinateReferenceSystem inputCrs = inputLayer->crs();

	// 创建一个临时图层，用于存储裁剪结果，设置 CRS 为输入图层的 CRS
	QString resultLayerCrs = QString("Polygon?crs=%1").arg(inputCrs.authid());
	QgsVectorLayer* resultLayer = new QgsVectorLayer(resultLayerCrs, "Clipped Layer", "memory");
	if (!resultLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("创建临时结果层失败!"));
		return false;
	}

	// 获取输入图层的字段
	QgsFields fields = inputLayer->fields();

	// 将 QgsFields 转换为 QList<QgsField>
	QList<QgsField> fieldList;
	for (int i = 0; i < fields.count(); ++i) {
		fieldList.append(fields.field(i));
	}

	// 添加字段到结果图层的数据提供程序中
	resultLayer->dataProvider()->addAttributes(fieldList);
	resultLayer->updateFields();

	// 遍历被裁剪图层的要素
	QgsFeatureIterator inputFeatures = inputLayer->getFeatures();
	QgsFeature inputFeature;

	while (inputFeatures.nextFeature(inputFeature)) {
		QgsGeometry inputGeometry = inputFeature.geometry();

		// 遍历裁剪图层的要素
		QgsFeatureIterator clipFeatures = clippingLayer->getFeatures();
		QgsFeature clipFeature;

		while (clipFeatures.nextFeature(clipFeature)) {
			QgsGeometry clipGeometry = clipFeature.geometry();

			// 计算交集
			QgsGeometry clippedGeometry = inputGeometry.intersection(clipGeometry);

			if (!clippedGeometry.isEmpty()) {
				// 创建裁剪后的要素
				QgsFeature clippedFeature(fields);
				clippedFeature.setGeometry(clippedGeometry);

				// 复制字段属性
				for (int i = 0; i < fields.count(); ++i) {
					clippedFeature.setAttribute(i, inputFeature.attribute(i));
				}

				// 添加到临时图层
				resultLayer->dataProvider()->addFeature(clippedFeature);
			}
		}
	}

	// 确保临时图层的 CRS 与输入图层一致
	resultLayer->setCrs(inputCrs);

	// 保存选项
	QgsVectorFileWriter::SaveVectorOptions saveOptions;
	saveOptions.driverName = "ESRI Shapefile";  // 保存格式
	saveOptions.fileEncoding = "UTF-8";        // 文件编码
	saveOptions.layerName = "ClippedLayer";    // 图层名称

	// 获取坐标转换上下文
	QgsCoordinateTransformContext transformContext = QgsProject::instance()->transformContext();

	// 使用 writeAsVectorFormatV3 保存结果
	QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormatV3(
		resultLayer,                  // 输入图层
		outputPath,                   // 输出文件路径
		transformContext,             // 坐标转换上下文
		saveOptions                   // 保存选项
	);

	// 检查保存结果
	if (error != QgsVectorFileWriter::NoError) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("保存剪切层到指定路径失败！"));
		delete resultLayer;
		return false;
	}

	// 删除临时图层以释放内存
	delete resultLayer;

	return true;
}

bool VectorClippingFunction::clippingTempVector(const QString& outputPath)
{
	if (!inputLayer || !clippingLayer) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("输入层和剪切层都必须在剪切之前加载！"));
		return false;
	}

	// 获取输入图层的坐标系
	QgsCoordinateReferenceSystem inputCrs = inputLayer->crs();

	// 创建一个临时图层用于存储裁剪结果，并设置为输入图层的坐标系
	QString wktCrs = inputCrs.toWkt();
	QgsVectorLayer* resultLayer = new QgsVectorLayer(QString("Polygon?crs=%1").arg(wktCrs), "Clipped Layer", "memory");
	if (!resultLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("创建结果层失败！"));
		return false;
	}

	// 获取输入图层的字段
	QgsFields fields = inputLayer->fields();
	resultLayer->dataProvider()->addAttributes(fields.toList());
	resultLayer->updateFields();

	// 遍历被裁剪图层的要素
	QgsFeatureIterator inputFeatures = inputLayer->getFeatures();
	QgsFeature inputFeature;

	while (inputFeatures.nextFeature(inputFeature)) {
		QgsGeometry inputGeometry = inputFeature.geometry();

		// 遍历裁剪图层的要素
		QgsFeatureIterator clipFeatures = clippingLayer->getFeatures();
		QgsFeature clipFeature;

		while (clipFeatures.nextFeature(clipFeature)) {
			QgsGeometry clipGeometry = clipFeature.geometry();

			// 计算交集
			QgsGeometry clippedGeometry = inputGeometry.intersection(clipGeometry);

			if (!clippedGeometry.isEmpty()) {
				// 创建裁剪后的要素
				QgsFeature clippedFeature(fields);
				clippedFeature.setGeometry(clippedGeometry);

				// 复制字段属性
				for (int i = 0; i < fields.count(); ++i) {
					clippedFeature.setAttribute(i, inputFeature.attribute(i));
				}

				// 添加到结果图层
				resultLayer->dataProvider()->addFeature(clippedFeature);
			}
		}
	}

	// 保存结果图层
	QgsVectorFileWriter::SaveVectorOptions saveOptions;
	saveOptions.driverName = "ESRI Shapefile";
	saveOptions.fileEncoding = "UTF-8";

	// 使用输入图层的坐标转换上下文
	QgsCoordinateTransformContext transformContext = QgsProject::instance()->transformContext();

	QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormatV3(
		resultLayer, outputPath, transformContext, saveOptions
	);

	if (error != QgsVectorFileWriter::NoError) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("保存剪切图层失败!"));
		delete resultLayer;
		return false;
	}

	delete resultLayer;
	return true;
}

void VectorClippingFunction::drawClippingVector(const QString& geometryType)
{
	// 获取选中的索引
	int currentIndex = ui.inputVectorBox->currentIndex();

	if (currentIndex < 0 || currentIndex >= loadedLayers.size()) {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("矢量图层不存在！"));
		return;
	}
	if (strInputFilePath.isEmpty()) {
		// 基于当前索引获取选中的图层
		inputLayer = loadedLayers[currentIndex];
	}
	
	DrawGeometryTool::DrawShape shape;
	if (geometryType == QStringLiteral("矩形")) {
		shape = DrawGeometryTool::Rectangle;
	}
	else if (geometryType == QStringLiteral("圆形")) {
		shape = DrawGeometryTool::Circle;
	}
	else if (geometryType == QStringLiteral("多边形")) {
		shape = DrawGeometryTool::Polygon;
	}
	else {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("无效的几何类型！"));
		return;
	}

	auto* drawTool = new DrawGeometryTool(mapCanvas, shape);
	connect(drawTool, &DrawGeometryTool::geometryDrawn, this, [this](const QgsGeometry& geometry) {
		if (!inputLayer) {
			QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("没有加载输入层来确定坐标参考系。"));
			return;
		}

		// 获取输入图层的坐标系
		const QgsCoordinateReferenceSystem crs = inputLayer->crs();

		// 创建临时图层并设置与输入图层相同的坐标系
		QString wktCrs = crs.toWkt();
		QgsVectorLayer* tempLayer = new QgsVectorLayer(QString("Polygon?crs=%1").arg(wktCrs), "Clipping Layer", "memory");
		if (!tempLayer->isValid()) {
			QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("创建临时层失败！"));
			return;
		}

		// 添加几何到图层
		QgsFeature feature;
		feature.setGeometry(geometry);
		tempLayer->dataProvider()->addFeature(feature);
		tempLayer->updateExtents();
		tempLayer->commitChanges();

		// 将临时图层添加到项目中
		QgsProject::instance()->addMapLayer(tempLayer);

		// 设置裁剪图层
		clippingLayer = tempLayer;

		// 获取当前图层列表并将新图层置于顶部
		QList<QgsMapLayer*> layers = mapCanvas->layers();
		layers.append(tempLayer);  // 添加新图层
		layers.move(layers.size() - 1, 0);  // 将新图层移动到顶部
		mapCanvas->setLayers(layers);  // 更新图层顺序

		// 更新地图显示范围
		mapCanvas->refresh();

		});

	mapCanvas->setMapTool(drawTool);
}

void VectorClippingFunction::onCreateClippingVector()
{
	QStringList options = { QStringLiteral("矩形"), QStringLiteral("圆形"), QStringLiteral("多边形") };
	bool ok;
	QString shapeType = QInputDialog::getItem(this, QStringLiteral("创建一个矢量面要素"), QStringLiteral("选择面要素类型:"), options, 0, false, &ok);

	if (ok && !shapeType.isEmpty()) {
		drawClippingVector(shapeType);
	}
}

void VectorClippingFunction::showMapLayerList() {
	// 创建一个标准项模型
	QStandardItemModel* model = new QStandardItemModel(this);

	// 获取项目中的所有图层
	const auto layers = QgsProject::instance()->mapLayers();

	// 遍历所有图层并将文件名添加到模型中
	for (const auto& layerPair : layers) {
		QgsMapLayer* layer = layerPair; // 提取图层对象

		QString displayName;

		if (auto vectorLayer = qobject_cast<QgsVectorLayer*>(layer)) {
			// 获取文件名
			QFileInfo fileInfo(vectorLayer->source());
			displayName = fileInfo.fileName(); // 提取文件名（包括扩展名）
		}
		else {
			// 如果不是矢量图层，直接使用图层名称
			displayName = layer->name();
		}

		QStandardItem* item = new QStandardItem(displayName);
		model->appendRow(item);
	}
}

void VectorClippingFunction::showMapLayerBox() {
	// 清空下拉框的内容以避免重复
	ui.inputVectorBox->clear();
	ui.clippiingVectorBox->clear();

	// 清空已加载图层列表
	loadedLayers.clear();

	// 获取项目中的所有图层
	const auto layers = QgsProject::instance()->mapLayers();

	for (const auto& layerPair : layers) {
		QgsMapLayer* layer = layerPair; // 提取图层对象
		if (layer->type() == Qgis::LayerType::Vector) {
			// 转换为 QgsVectorLayer
			if (auto vectorLayer = qobject_cast<QgsVectorLayer*>(layer)) {
				// 获取图层文件路径
				QString sourcePath = vectorLayer->source();

				// 提取文件名
				QFileInfo fileInfo(sourcePath);
				QString fileName = fileInfo.fileName(); // 获取文件名（带扩展名）

				// 添加文件名到下拉框
				ui.inputVectorBox->addItem(fileName);
				ui.clippiingVectorBox->addItem(fileName);

				// 添加到已加载图层列表
				loadedLayers.append(vectorLayer);

				QgsProject::instance()->addMapLayer(layer);
				mapCanvas->refresh();
			}
		}
	}
}




