#include "PlaneOfDivision.h"

PlaneOfDivision::PlaneOfDivision(QgsMapCanvas* m_mapCanvas, QWidget* parent)
	: QMainWindow(parent), m_mapCanvas(m_mapCanvas)
{
	ui.setupUi(this);

	setStyle();

	setWindowIcon(QIcon(":/MainInterface/res/surface.png"));
	setWindowTitle(QStringLiteral("分割面"));

	mapCanvas = new QgsMapCanvas();
	mapCanvas->setCanvasColor(Qt::white);  // 设置背景颜色
	mapCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);  // 设置大小策略以适应滚动区域

	// 将 QgsMapCanvas 放入 QScrollArea 中
	ui.showSurfaceArea->setWidget(mapCanvas);

	// 确保项目已加载
	QgsProject::instance();

	updateView();
	showMapLayerBox();

	// 连接主画布的范围变更信号
	connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &PlaneOfDivision::updateView);
	connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &PlaneOfDivision::showMapLayerBox);

	// 连接按钮事件
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
	// 打开文件对话框选择矢量文件
	strInputFilePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择矢量面要素"), "", QStringLiteral("Shapefiles (*.shp);;All Files (*)"));
	if (!strInputFilePath.isEmpty()) {
		showVectorData(strInputFilePath);
	}
}

void PlaneOfDivision::showVectorData(const QString& filePath)
{
	// 创建矢量图层并加载文件
	inputLayer = new QgsVectorLayer(filePath, QFileInfo(filePath).baseName(), "ogr");
	if (!inputLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("加载失败"), QStringLiteral("无法加载矢量图层！"));
		return;
	}

	// 将图层添加到 QgsProject 中
	QgsProject::instance()->addMapLayer(inputLayer);

	// 获取当前 mapCanvas 图层列表
	QList<QgsMapLayer*> layers = mapCanvas->layers();
	layers.append(inputLayer);  // 将新图层添加到图层列表中
	mapCanvas->setLayers(layers);  // 更新图层列表

	// 自动调整视图范围以适应新图层
	mapCanvas->zoomToFullExtent();

	// 将新图层移动到顶部
	layers.move(layers.size() - 1, 0);
	mapCanvas->setLayers(layers);  // 更新图层顺序

	// 刷新 mapCanvas
	mapCanvas->refresh();
}

void PlaneOfDivision::onFeatureIdentified(const QgsFeature& feature)
{
	if (!isSelecting) return;

	if (!inputLayer || !inputLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("矢量图层无效！"));
		return;
	}

	// 检查是否选中有效的面要素
	if (feature.geometry().wkbType() != Qgis::WkbType::Polygon &&
		feature.geometry().wkbType() != Qgis::WkbType::MultiPolygon) {
		QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请选择一个面要素！"));
		return;
	}

	// 将选中的要素保存并结束选择模式
	selectedFeatures.append(feature);
	isSelecting = false;

	// 提示用户
	QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已选中面要素，请继续绘制一个多边形。"));

	// 调用绘制多边形的逻辑
	showDivisionData();
}

void PlaneOfDivision::onCreateDivisionClicked()
{
	if (!isSelecting) {
		isSelecting = true;  // 开启选择模式
		selectedFeatures.clear();  // 清空已选择的要素
	}

	// 查找当前地图画布的矢量图层
	inputLayer = nullptr;
	for (QgsMapLayer* mapLayer : mapCanvas->layers()) {
		if (mapLayer->type() == Qgis::LayerType::Vector) {
			inputLayer = qobject_cast<QgsVectorLayer*>(mapLayer);
			break;
		}
	}

	if (!inputLayer || !inputLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("未找到有效的矢量图层！"));
		return;
	}

	// 初始化 identifyTool 并连接信号槽
	if (!identifyTool) {
		identifyTool = new QgsMapToolIdentifyFeature(mapCanvas, inputLayer);
		connect(identifyTool, SIGNAL(featureIdentified(const QgsFeature&)), this, SLOT(onFeatureIdentified(const QgsFeature&)));
	}

	// 设置识别工具为当前地图工具
	mapCanvas->setMapTool(identifyTool);

	// 提示用户选择操作
	QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请选择一个面要素，完成后将继续下一步操作。"));
}

void PlaneOfDivision::highlightSelectedFeatures(const QList<QgsFeature>& features)
{
	if (!rubberBand) {
		// 初始化 rubberBand 为多边形高亮
		rubberBand = new QgsRubberBand(mapCanvas, Qgis::GeometryType::Polygon);
		rubberBand->setColor(Qt::yellow); // 设置高亮颜色
		rubberBand->setWidth(200);       // 设置线宽
	}

	rubberBand->reset(Qgis::GeometryType::Polygon);  // 重置高亮区域

	if (!inputLayer) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("无效的输入图层！"));
		return;
	}

	QgsCoordinateTransform transform(inputLayer->crs(), mapCanvas->mapSettings().destinationCrs(), QgsProject::instance());

	for (const QgsFeature& feature : features) {
		if (!feature.isValid()) {
			continue;
		}

		QgsGeometry geom = feature.geometry();
		if (geom.isNull() || !geom.isGeosValid()) {
			QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("几何无效，无法高亮！"));
			continue;
		}

		// 获取几何的单一类型（例如将MultiPolygon转换为Polygon）
		Qgis::WkbType singleType = QgsWkbTypes::singleType(geom.wkbType());

		// 检查几何是否是支持的类型
		if (singleType == Qgis::WkbType::Polygon || singleType == Qgis::WkbType::LineString) {
			// 转换几何到画布的目标坐标参考系统
			QgsGeometry transformedGeom = geom;
			transformedGeom.transform(transform);
			rubberBand->addGeometry(transformedGeom, nullptr); // 添加几何
		}
		else {
			QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("几何类型不支持高亮显示！"));
		}
	}

	rubberBand->show(); // 确保高亮显示
	mapCanvas->refresh(); // 刷新画布
}

void PlaneOfDivision::onActionDivisionClicked()
{
	// 调用 actionDivision 函数进行分割操作
	actionDivision();
}

void PlaneOfDivision::actionDivision()
{
	// 获取选中的索引
	int currentIndex = ui.comboBox->currentIndex();

	if (currentIndex < 0 || currentIndex >= loadedLayers.size()) {
		QMessageBox::critical(this, tr("Error"), QStringLiteral("矢量图层不存在！"));
		return;
	}
	if (strInputFilePath.isEmpty()) {
		// 基于当前索引获取选中的图层
		inputLayer = loadedLayers[currentIndex];
	}
	if (!inputLayer || !DivisionLayer) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("输入图层或分割图层未加载！"));
		return;
	}

	if (!inputLayer->isValid() || !DivisionLayer->isValid()) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("输入图层或分割图层无效！"));
		return;
	}

	// 确保两个图层都是面图层
	if (inputLayer->geometryType() != Qgis::GeometryType::Polygon || DivisionLayer->geometryType() != Qgis::GeometryType::Polygon) {
		QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("输入图层和分割图层都必须是面图层！"));
		return;
	}

	QgsFeatureList newFeatures; // 用于存储分割后的新要素
	QgsFeatureIterator inputIterator = inputLayer->getFeatures(); // 获取目标图层的要素迭代器

	QgsFeature inputFeature;
	while (inputIterator.nextFeature(inputFeature)) { // 遍历 inputLayer 的每个要素
		QgsGeometry inputGeometry = inputFeature.geometry();
		if (inputGeometry.isNull() || !inputGeometry.isGeosValid()) continue;

		QgsFeatureIterator divisionIterator = DivisionLayer->getFeatures(); // 获取分割图层的要素迭代器
		QgsFeature divisionFeature;
		QgsGeometry currentGeometry = inputGeometry;

		// 遍历分割图层中的要素
		while (divisionIterator.nextFeature(divisionFeature)) {
			QgsGeometry divisionGeometry = divisionFeature.geometry();
			if (divisionGeometry.isNull() || !divisionGeometry.isGeosValid()) continue;

			// 检查是否相交
			if (!currentGeometry.intersects(divisionGeometry)) {
				continue;
			}

			// 计算相交部分
			QgsGeometry intersectedGeometry = currentGeometry.intersection(divisionGeometry);
			if (!intersectedGeometry.isNull() && intersectedGeometry.isGeosValid()) {
				// 创建新的要素保存相交部分
				QgsFeature newFeature(inputFeature.fields());
				newFeature.setGeometry(intersectedGeometry);
				newFeatures.append(newFeature);
			}

			// 更新当前几何为未相交部分
			currentGeometry = currentGeometry.difference(divisionGeometry);
		}

		// 如果仍有未相交部分，更新到原图层
		if (!currentGeometry.isNull() && currentGeometry.isGeosValid()) {
			inputFeature.setGeometry(currentGeometry);
			inputLayer->updateFeature(inputFeature);
		}
	}

	// 将新要素添加到输入图层
	if (!newFeatures.isEmpty()) {
		inputLayer->dataProvider()->addFeatures(newFeatures);
	}

	// 刷新输入图层
	inputLayer->triggerRepaint();

	QMessageBox::information(this, QStringLiteral("完成"), QStringLiteral("分割操作完成！"));

}

void PlaneOfDivision::showDivisionData() {

	QStringList options = { QStringLiteral("矩形"), QStringLiteral("圆形"), QStringLiteral("多边形") };
	bool ok;
	QString shapeType = QInputDialog::getItem(this, QStringLiteral("创建一个矢量面要素"), QStringLiteral("选择面要素类型:"), options, 0, false, &ok);

	if (ok && !shapeType.isEmpty()) {
		createDivisionPolygon(shapeType);
	}

}

void PlaneOfDivision::createDivisionPolygon(const QString& geometryType) {
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

		// 设置分割图层
		DivisionLayer = tempLayer;

		// 获取当前图层列表并将新图层置于顶部
		QList<QgsMapLayer*> layers = mapCanvas->layers();
		layers.append(tempLayer);  // 添加新图层
		mapCanvas->setLayers(layers);  // 更新图层顺序

		// 更新地图显示范围
		mapCanvas->refresh();

		});

	mapCanvas->setMapTool(drawTool);


}

void PlaneOfDivision::showMapLayerBox() {
	// 清空下拉框的内容以避免重复
	ui.comboBox->clear();

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
				ui.comboBox->addItem(fileName);

				// 添加到已加载图层列表
				loadedLayers.append(vectorLayer);

				QgsProject::instance()->addMapLayer(layer);
				mapCanvas->refresh();
			}
		}
	}
}
