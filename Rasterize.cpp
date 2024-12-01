#include "Rasterize.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QgsVectorLayer.h>
#include <QgsProcessing.h>
#include <QgsProcessingFeedback.h>
#include <QgsApplication.h>
#include <QgsRasterFileWriter.h>
#include <QgsProcessingRegistry.h>
#include <QgsProcessingAlgorithm.h>
#include <qgsnativealgorithms.h>
#include <qdebug.h>
#include <QgsWkbTypes.h>
#include <QtCore>        
#include <algorithm>
#include <QgsCoordinateReferenceSystem.h>
#include <QgsCoordinateTransform.h> 
#include "gdal_priv.h" 
#include "ogrsf_frmts.h" 
#include "qgis.h"

void initializeProcessing() {
    QgsApplication::processingRegistry()->addProvider(new QgsNativeAlgorithms());
    // ��ȡ�����㷨
    const auto algorithms = QgsApplication::processingRegistry()->algorithms();
    for (const QgsProcessingAlgorithm* alg : algorithms) {
        qDebug() << "Algorithm ID:" << alg->id() << ", Name:" << alg->displayName();
    }
}

Rasterize::Rasterize(QgsMapCanvas* m_mapCanvas, QWidget* parent)
    : QMainWindow(parent), m_mapCanvas(m_mapCanvas)
{
    QgsApplication::init();
    QgsApplication::initQgis();
    // ע�ᴦ���㷨
    initializeProcessing();
   
    ui.setupUi(this);

    setWindowIcon(QIcon(":/MainInterface/res/rasterize.png"));
    setWindowTitle(QStringLiteral("ʸ��תդ��"));

    setStyle();

    // ��ʼ�� GDAL
    GDALAllRegister();
    ui.pixelSizeLineEdit->setText("0.0013");   //��ʼ��Ĭ����Ԫ��С
    updateComboBox();

    connect(ui.fileComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &Rasterize::onCurrentItemChanged);
    connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, &Rasterize::updateComboBox);
}

Rasterize::~Rasterize()
{
    if (m_vectorLayer)
    {
        delete m_vectorLayer;
        m_vectorLayer = nullptr;
    }
}

void Rasterize::updateComboBox() {
    // �����Ͽ�
    ui.fieldComboBox->clear();
    m_vectorLayers.clear(); // ȷ�������ǰ��ͼ��

    // ��ȡ��Ŀ�е�����ͼ��
    const auto layers = QgsProject::instance()->mapLayers();
    for (const auto& layer : layers) {
        if (layer->type() == Qgis::LayerType::Vector) {
            ui.fileComboBox->addItem(layer->name());
            loadVectorLayer(qobject_cast<QgsVectorLayer*>(layer));
            m_vectorLayers.append(qobject_cast<QgsVectorLayer*>(layer));
        }
    }
}

//ѡ��ʸ������
void Rasterize::on_browseVectorFileButton_clicked() {
    QStringList vectorFilePaths = QFileDialog::getOpenFileNames(
        this, tr("ѡ��ʸ���ļ�"), "", "Shapefiles (*.shp *.geojson *.kml)");

    if (vectorFilePaths.isEmpty()) {
        return;
    }

    for (const QString& filePath : vectorFilePaths) {
        QgsVectorLayer* vecLayer = new QgsVectorLayer(filePath);
        // ����ѡ���ʸ���ļ�
        loadVectorLayer(vecLayer); // �����º���������ͼ��
    }
    ui.vectorFileLineEdit->setText(m_vectorLayer->source());
}

void Rasterize::onCurrentItemChanged() {
    // ��ȡѡ�е�����
    int currentIndex = ui.fieldComboBox->currentIndex();

    if (currentIndex < 0 || currentIndex >= m_vectorLayers.size()) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("ʸ��ͼ�㲻���ڣ�"));
        return;
    }

    // ���ڵ�ǰ������ȡѡ�е�ͼ��
    QgsVectorLayer* currentLayer = m_vectorLayers[currentIndex];

    // ���͸��µ��ֶ���Ͽ�
    updateFieldComboBox(currentLayer); // �����ֶε�����������
}

// �����ֶ�������ĺ���
void Rasterize::updateFieldComboBox(QgsVectorLayer* vectorLayer) {
    ui.fieldComboBox->clear(); // ���������
    QgsFields fields = vectorLayer->fields();
    for (int i = 0; i < fields.count(); ++i) {
        ui.fieldComboBox->addItem(fields.at(i).name());
    }
}

void Rasterize::loadVectorLayer(QgsVectorLayer* vectorLayer) {
    if (!vectorLayer || !vectorLayer->isValid()) {
        QMessageBox::warning(this, tr("Error"), tr("����ʸ��ͼ��ʧ�ܣ�"));
        delete vectorLayer; // �ͷ���Чͼ��
        return;
    }

    // ���û����Ч�� CRS���ֶ�����һ��Ĭ�� CRS������ EPSG:4326��
    if (!vectorLayer->crs().isValid()) {
        QgsCoordinateReferenceSystem crs("EPSG:4326"); // ����Ĭ�� CRS
        vectorLayer->setCrs(crs); // ���� CRS
        qDebug() << "CRS was invalid. Setting to EPSG:4326";
    }

    // ��� CRS ��Ϣ
    QgsCoordinateReferenceSystem crs = vectorLayer->crs();
    qDebug() << "CRS of vector layer: " << crs.authid();  // ��� CRS

    // ����ͼ�㲢�����ֶ�������
    m_vectorLayer = vectorLayer; // ����ǰͼ�㸳ֵ�� m_vectorLayer

    ui.fieldComboBox->clear(); // ���������
    QgsFields fields = vectorLayer->fields();
    for (int i = 0; i < fields.count(); ++i) {
        ui.fieldComboBox->addItem(fields.at(i).name());
    }
}

//����դ���ļ�·��
void Rasterize::on_browseRasterFileButton_clicked()
{
    m_outputFilePath = QFileDialog::getSaveFileName(this, QStringLiteral("ѡ�񱣴�·��"), "", "GeoTIFF Files (*.tif)");
    if (m_outputFilePath.isEmpty())
        return;

    ui.rasterFileLineEdit->setText(m_outputFilePath);
}

void Rasterize::on_convertToRasterButton_clicked()
{
    // ��ȡѡ�е�����
    int currentIndex = ui.fileComboBox->currentIndex();

    if (currentIndex < 0 || currentIndex >= m_vectorLayers.size()) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("ʸ��ͼ�㲻���ڣ�"));
        return;
    }

    // ���ڵ�ǰ������ȡѡ�е�ͼ��
    m_vectorLayer = m_vectorLayers[currentIndex];

    // ȷ����������Ч��ʸ��ͼ��
    if (!m_vectorLayer || !m_vectorLayer->isValid()) {
        QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("ʸ��ͼ����Ч��"));
        return;
    }

    // ��ȡʸ��ͼ�������ο�ϵͳ
    QgsCoordinateReferenceSystem crs = m_vectorLayer->crs();

    // ��ȡ����ļ�·��
    QString outputFilePath = ui.rasterFileLineEdit->text();
    if (outputFilePath.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("û�����·����"));
        return;
    }

    // ��ȡʸ��ͼ�㷶Χ��EXTENT��
    QgsRectangle extent = m_vectorLayer->extent();

    // �����Ҫ�����Խ�������ο�ϵͳת��
    QgsCoordinateReferenceSystem targetCRS = QgsCoordinateReferenceSystem("EPSG:4326");  // Ŀ�� CRS������ WGS 84��
    if (crs != targetCRS) {
        QgsCoordinateTransform transform(crs, targetCRS, QgsProject::instance());
        extent = transform.transform(extent);  // ת����Χ��Ŀ�� CRS
    }

    QString extentString = QString("%1,%2,%3,%4")
        .arg(extent.xMinimum())
        .arg(extent.xMaximum())
        .arg(extent.yMinimum())
        .arg(extent.yMaximum());

    // ��ȡ�û��������Ԫ��С����lineEdit��ȡ��
    bool isok;
    double pixelSize = ui.pixelSizeLineEdit->text().toDouble(&isok);  // ����lineEdit�Ķ�����ΪpixelSizeLineEdit
    if (!isok || pixelSize <= 0) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("��Ԫ��С��Ч��"));
        return;
    }


    int tileSize = qMin(1024, std::max(64, static_cast<int>(qSqrt(extent.width() * extent.height()) / 2)));

    // ����դ�񻯲���
    QVariantMap parameters;
    parameters.insert("INPUT", m_vectorLayer->source());
    parameters.insert("FIELD", ui.fieldComboBox->currentText());
    parameters.insert("EXTENT", extentString);                             // դ�񻯷�Χ
    parameters.insert("MAP_UNITS_PER_PIXEL", pixelSize);                   // ÿ���صĵ�ͼ��λ
    parameters.insert("OUTPUT", outputFilePath);
    parameters.insert("TILE_SIZE", tileSize);                                // ��̬�����ͼ���С


    // ��ȡդ���㷨
    QgsProcessingAlgorithm* alg = QgsApplication::processingRegistry()->createAlgorithmById("native:rasterize");
    if (!alg) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("�޷�����դ���㷨��"));
        return;
    }

    // ��������������
    QgsProcessingContext context;
    QgsProject* project = QgsProject::instance();
    project->setCrs(crs);  // ʹ��ʸ��ͼ��� CRS
    project->addMapLayer(m_vectorLayer);
    context.setProject(project);

    QgsProcessingFeedback feedback;

    // ׼���������㷨
    if (!alg->prepare(parameters, context, &feedback)) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("�㷨����ʧ�ܣ�"));
        delete alg;
        return;
    }

    bool ok = false;
    QVariantMap results = alg->run(parameters, context, &feedback, &ok);

    // ���ִ�н��
    if (!ok) {
        QString feedbackLog = feedback.textLog();
        QMessageBox::critical(this, tr("Error"), QStringLiteral("դ��ʧ�� Log:\n") + feedbackLog);
        delete alg;
        return;
    }

    // �������ļ��Ƿ����
    if (!QFile::exists(outputFilePath)) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("������ָ��·��ʧ�ܣ�"));
        delete alg;
        return;
    }


    QFileInfo fi(m_outputFilePath);
    if (!fi.exists()) { return; }
    QString layerBaseName = fi.baseName(); // ͼ������

    QgsRasterLayer* resultLayer = new QgsRasterLayer(m_outputFilePath, layerBaseName);

    QgsProject::instance()->addMapLayer(resultLayer);
    delete alg;
}

void Rasterize::setStyle() {
    style.setConfirmButton(ui.convertToRasterButton);

    style.setCommonButton(ui.browseVectorFileButton);

    style.setCommonButton(ui.browseRasterFileButton);

    style.setLineEdit(ui.pixelSizeLineEdit);

    style.setLineEdit(ui.rasterFileLineEdit);

    style.setLineEdit(ui.vectorFileLineEdit);

    style.setComboBox(ui.fileComboBox);

    style.setComboBox(ui.fieldComboBox);
}
