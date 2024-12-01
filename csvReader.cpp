#pragma execution_character_set("utf-8")
#include "csvReader.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <QgsVectorLayer.h>
#include <QgsProject.h>

csvReader::csvReader(QgsMapCanvas* mapCanvas, QObject* parent) : QObject(parent) {
}
//打开csv文件（打开查找窗口）
QgsVectorLayer* csvReader::openCSVFile() {
	QString csvFilePath = QFileDialog::getOpenFileName(nullptr, QStringLiteral("选择 CSV 文件"), "", "CSV 文件 (*.csv)");
	if (csvFilePath.isEmpty()) {
		return nullptr;
	}

	QString shpOutputPath = QFileInfo(csvFilePath).baseName() + "_shp.shp";
	return convertCSVToShapefile(csvFilePath, shpOutputPath);
}
//将csv文件转成shp文件
QgsVectorLayer* csvReader::convertCSVToShapefile(const QString& csvFilePath, const QString& shpOutputPath) {
	// 检查 CSV 文件是否存在
	QFileInfo csvInfo(csvFilePath);
	if (!csvInfo.exists() || !csvInfo.isFile()) {
		QMessageBox::critical(nullptr, tr("错误"), tr("CSV 文件不存在：%1").arg(csvFilePath));
		return nullptr;
	}
	// 检查输出路径是否有效
	QFileInfo shpInfo(shpOutputPath);
	QDir outputDir = shpInfo.dir();
	if (!outputDir.exists()) {
		QMessageBox::critical(nullptr, tr("错误"), tr("输出目录不存在：%1").arg(outputDir.path()));
		return nullptr;
	}
	// 初始化 GDAL
	GDALAllRegister();
	// 打开 CSV 数据源，设置支持中文的编码选项
	char** options = nullptr;
	options = CSLAddNameValue(options, "ENCODING", "GBK");
	GDALDataset* csvDataset = static_cast<GDALDataset*>(GDALOpenEx(
		csvFilePath.toLocal8Bit().constData(), GDAL_OF_VECTOR, nullptr, options, nullptr));
	CSLDestroy(options);

	if (!csvDataset) {
		QMessageBox::critical(nullptr, tr("错误"), tr("无法打开 CSV 文件：%1").arg(csvFilePath));
		qDebug() << "GDAL 打开 CSV 失败：" << csvFilePath;
		return nullptr;
	}
	// 创建输出 SHP 数据源
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
	if (!driver) {
		QMessageBox::critical(nullptr, tr("错误"), tr("无法获取 GDAL 驱动 ESRI Shapefile！"));
		GDALClose(csvDataset);
		return nullptr;
	}

	GDALDataset* shpDataset = driver->Create(
		shpOutputPath.toLocal8Bit().constData(),
		0, 0, 0,
		GDT_Unknown,
		nullptr
	);

	if (!shpDataset) {
		QMessageBox::critical(nullptr, tr("错误"), tr("无法创建 SHP 文件：%1").arg(shpOutputPath));
		GDALClose(csvDataset);
		return nullptr;
	}
	// 创建图层并设置编码
	char** layerOptions = nullptr;
	layerOptions = CSLAddNameValue(layerOptions, "ENCODING", "GBK");
	OGRLayer* layer = shpDataset->CreateLayer("points", nullptr, wkbPoint, layerOptions);
	CSLDestroy(layerOptions);

	if (!layer) {
		QMessageBox::critical(nullptr, tr("错误"), tr("无法创建 SHP 图层！"));
		GDALClose(shpDataset);
		GDALClose(csvDataset);
		return nullptr;
	}
	// 获取 CSV 图层
	OGRLayer* csvLayer = csvDataset->GetLayer(0);
	if (!csvLayer) {
		QMessageBox::critical(nullptr, tr("错误"), tr("无法读取 CSV 图层！"));
		GDALClose(shpDataset);
		GDALClose(csvDataset);
		return nullptr;
	}
	// 动态添加字段
	OGRFeatureDefn* csvFeatureDefn = csvLayer->GetLayerDefn();
	for (int i = 0; i < csvFeatureDefn->GetFieldCount(); ++i) {
		OGRFieldDefn* csvFieldDefn = csvFeatureDefn->GetFieldDefn(i);
		OGRFieldDefn newFieldDefn(csvFieldDefn->GetNameRef(), csvFieldDefn->GetType());
		if (layer->CreateField(&newFieldDefn) != OGRERR_NONE) {
			QMessageBox::critical(nullptr, tr("错误"), tr("无法创建字段：%1").arg(csvFieldDefn->GetNameRef()));
			GDALClose(shpDataset);
			GDALClose(csvDataset);
			return nullptr;
		}
	}
	// 获取新图层的字段定义
	OGRFeatureDefn* shpFeatureDefn = layer->GetLayerDefn();
	// 遍历 CSV 数据并将其转换为 SHP
	OGRFeature* csvFeature;
	while ((csvFeature = csvLayer->GetNextFeature()) != nullptr) {
		// 创建新的 SHP 特征
		OGRFeature* newFeature = OGRFeature::CreateFeature(shpFeatureDefn);
		// 复制属性表字段内容
		for (int i = 0; i < csvFeatureDefn->GetFieldCount(); ++i) {
			newFeature->SetField(i, csvFeature->GetFieldAsString(i));
		}
		// 创建点几何体
		OGRPoint point;
		point.setX(csvFeature->GetFieldAsDouble("locationx"));
		point.setY(csvFeature->GetFieldAsDouble("locationy"));
		newFeature->SetGeometry(&point);
		// 添加特性到图层
		if (layer->CreateFeature(newFeature) != OGRERR_NONE) {
			qDebug() << "无法创建特性";
		}
		// 清理特性
		OGRFeature::DestroyFeature(newFeature);
		OGRFeature::DestroyFeature(csvFeature);
	}
	// 清理资源
	GDALClose(shpDataset);
	GDALClose(csvDataset);
	// 加载生成的 SHP 文件为矢量图层
	return loadShapefile(shpOutputPath);
}
//加载转成的shp文件
QgsVectorLayer* csvReader::loadShapefile(const QString& shpFilePath) {
	// 创建矢量图层
	QFileInfo shpInfo(shpFilePath);
	QgsVectorLayer* vectorLayer = new QgsVectorLayer(shpFilePath, shpInfo.baseName(), "ogr");
	// 检查图层有效性
	if (!vectorLayer->isValid()) {
		QMessageBox::critical(nullptr, tr("错误"), tr("无法加载 SHP 文件为矢量图层：%1").arg(shpFilePath));
		delete vectorLayer;
		return nullptr;
	}
	return vectorLayer;

}
