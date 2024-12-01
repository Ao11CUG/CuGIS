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
//��csv�ļ����򿪲��Ҵ��ڣ�
QgsVectorLayer* csvReader::openCSVFile() {
	QString csvFilePath = QFileDialog::getOpenFileName(nullptr, QStringLiteral("ѡ�� CSV �ļ�"), "", "CSV �ļ� (*.csv)");
	if (csvFilePath.isEmpty()) {
		return nullptr;
	}

	QString shpOutputPath = QFileInfo(csvFilePath).baseName() + "_shp.shp";
	return convertCSVToShapefile(csvFilePath, shpOutputPath);
}
//��csv�ļ�ת��shp�ļ�
QgsVectorLayer* csvReader::convertCSVToShapefile(const QString& csvFilePath, const QString& shpOutputPath) {
	// ��� CSV �ļ��Ƿ����
	QFileInfo csvInfo(csvFilePath);
	if (!csvInfo.exists() || !csvInfo.isFile()) {
		QMessageBox::critical(nullptr, tr("����"), tr("CSV �ļ������ڣ�%1").arg(csvFilePath));
		return nullptr;
	}
	// ������·���Ƿ���Ч
	QFileInfo shpInfo(shpOutputPath);
	QDir outputDir = shpInfo.dir();
	if (!outputDir.exists()) {
		QMessageBox::critical(nullptr, tr("����"), tr("���Ŀ¼�����ڣ�%1").arg(outputDir.path()));
		return nullptr;
	}
	// ��ʼ�� GDAL
	GDALAllRegister();
	// �� CSV ����Դ������֧�����ĵı���ѡ��
	char** options = nullptr;
	options = CSLAddNameValue(options, "ENCODING", "GBK");
	GDALDataset* csvDataset = static_cast<GDALDataset*>(GDALOpenEx(
		csvFilePath.toLocal8Bit().constData(), GDAL_OF_VECTOR, nullptr, options, nullptr));
	CSLDestroy(options);

	if (!csvDataset) {
		QMessageBox::critical(nullptr, tr("����"), tr("�޷��� CSV �ļ���%1").arg(csvFilePath));
		qDebug() << "GDAL �� CSV ʧ�ܣ�" << csvFilePath;
		return nullptr;
	}
	// ������� SHP ����Դ
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
	if (!driver) {
		QMessageBox::critical(nullptr, tr("����"), tr("�޷���ȡ GDAL ���� ESRI Shapefile��"));
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
		QMessageBox::critical(nullptr, tr("����"), tr("�޷����� SHP �ļ���%1").arg(shpOutputPath));
		GDALClose(csvDataset);
		return nullptr;
	}
	// ����ͼ�㲢���ñ���
	char** layerOptions = nullptr;
	layerOptions = CSLAddNameValue(layerOptions, "ENCODING", "GBK");
	OGRLayer* layer = shpDataset->CreateLayer("points", nullptr, wkbPoint, layerOptions);
	CSLDestroy(layerOptions);

	if (!layer) {
		QMessageBox::critical(nullptr, tr("����"), tr("�޷����� SHP ͼ�㣡"));
		GDALClose(shpDataset);
		GDALClose(csvDataset);
		return nullptr;
	}
	// ��ȡ CSV ͼ��
	OGRLayer* csvLayer = csvDataset->GetLayer(0);
	if (!csvLayer) {
		QMessageBox::critical(nullptr, tr("����"), tr("�޷���ȡ CSV ͼ�㣡"));
		GDALClose(shpDataset);
		GDALClose(csvDataset);
		return nullptr;
	}
	// ��̬����ֶ�
	OGRFeatureDefn* csvFeatureDefn = csvLayer->GetLayerDefn();
	for (int i = 0; i < csvFeatureDefn->GetFieldCount(); ++i) {
		OGRFieldDefn* csvFieldDefn = csvFeatureDefn->GetFieldDefn(i);
		OGRFieldDefn newFieldDefn(csvFieldDefn->GetNameRef(), csvFieldDefn->GetType());
		if (layer->CreateField(&newFieldDefn) != OGRERR_NONE) {
			QMessageBox::critical(nullptr, tr("����"), tr("�޷������ֶΣ�%1").arg(csvFieldDefn->GetNameRef()));
			GDALClose(shpDataset);
			GDALClose(csvDataset);
			return nullptr;
		}
	}
	// ��ȡ��ͼ����ֶζ���
	OGRFeatureDefn* shpFeatureDefn = layer->GetLayerDefn();
	// ���� CSV ���ݲ�����ת��Ϊ SHP
	OGRFeature* csvFeature;
	while ((csvFeature = csvLayer->GetNextFeature()) != nullptr) {
		// �����µ� SHP ����
		OGRFeature* newFeature = OGRFeature::CreateFeature(shpFeatureDefn);
		// �������Ա��ֶ�����
		for (int i = 0; i < csvFeatureDefn->GetFieldCount(); ++i) {
			newFeature->SetField(i, csvFeature->GetFieldAsString(i));
		}
		// �����㼸����
		OGRPoint point;
		point.setX(csvFeature->GetFieldAsDouble("locationx"));
		point.setY(csvFeature->GetFieldAsDouble("locationy"));
		newFeature->SetGeometry(&point);
		// ������Ե�ͼ��
		if (layer->CreateFeature(newFeature) != OGRERR_NONE) {
			qDebug() << "�޷���������";
		}
		// ��������
		OGRFeature::DestroyFeature(newFeature);
		OGRFeature::DestroyFeature(csvFeature);
	}
	// ������Դ
	GDALClose(shpDataset);
	GDALClose(csvDataset);
	// �������ɵ� SHP �ļ�Ϊʸ��ͼ��
	return loadShapefile(shpOutputPath);
}
//����ת�ɵ�shp�ļ�
QgsVectorLayer* csvReader::loadShapefile(const QString& shpFilePath) {
	// ����ʸ��ͼ��
	QFileInfo shpInfo(shpFilePath);
	QgsVectorLayer* vectorLayer = new QgsVectorLayer(shpFilePath, shpInfo.baseName(), "ogr");
	// ���ͼ����Ч��
	if (!vectorLayer->isValid()) {
		QMessageBox::critical(nullptr, tr("����"), tr("�޷����� SHP �ļ�Ϊʸ��ͼ�㣺%1").arg(shpFilePath));
		delete vectorLayer;
		return nullptr;
	}
	return vectorLayer;

}
