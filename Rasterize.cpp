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
    // 获取所有算法
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
    // 注册处理算法
    initializeProcessing();
   
    ui.setupUi(this);

    setWindowIcon(QIcon(":/MainInterface/res/rasterize.png"));
    setWindowTitle(QStringLiteral("矢量转栅格"));

    setStyle();

    // 初始化 GDAL
    GDALAllRegister();
    ui.pixelSizeLineEdit->setText("0.0013");   //初始化默认像元大小
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
    // 清空组合框
    ui.fieldComboBox->clear();
    m_vectorLayers.clear(); // 确保清空先前的图层

    // 获取项目中的所有图层
    const auto layers = QgsProject::instance()->mapLayers();
    for (const auto& layer : layers) {
        if (layer->type() == Qgis::LayerType::Vector) {
            ui.fileComboBox->addItem(layer->name());
            loadVectorLayer(qobject_cast<QgsVectorLayer*>(layer));
            m_vectorLayers.append(qobject_cast<QgsVectorLayer*>(layer));
        }
    }
}

//选择矢量数据
void Rasterize::on_browseVectorFileButton_clicked() {
    QStringList vectorFilePaths = QFileDialog::getOpenFileNames(
        this, tr("选择矢量文件"), "", "Shapefiles (*.shp *.geojson *.kml)");

    if (vectorFilePaths.isEmpty()) {
        return;
    }

    for (const QString& filePath : vectorFilePaths) {
        QgsVectorLayer* vecLayer = new QgsVectorLayer(filePath);
        // 处理选择的矢量文件
        loadVectorLayer(vecLayer); // 调用新函数来加载图层
    }
    ui.vectorFileLineEdit->setText(m_vectorLayer->source());
}

void Rasterize::onCurrentItemChanged() {
    // 获取选中的索引
    int currentIndex = ui.fieldComboBox->currentIndex();

    if (currentIndex < 0 || currentIndex >= m_vectorLayers.size()) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("矢量图层不存在！"));
        return;
    }

    // 基于当前索引获取选中的图层
    QgsVectorLayer* currentLayer = m_vectorLayers[currentIndex];

    // 发送更新到字段组合框
    updateFieldComboBox(currentLayer); // 更新字段的下拉框内容
}

// 更新字段下拉框的函数
void Rasterize::updateFieldComboBox(QgsVectorLayer* vectorLayer) {
    ui.fieldComboBox->clear(); // 清空下拉框
    QgsFields fields = vectorLayer->fields();
    for (int i = 0; i < fields.count(); ++i) {
        ui.fieldComboBox->addItem(fields.at(i).name());
    }
}

void Rasterize::loadVectorLayer(QgsVectorLayer* vectorLayer) {
    if (!vectorLayer || !vectorLayer->isValid()) {
        QMessageBox::warning(this, tr("Error"), tr("加载矢量图层失败！"));
        delete vectorLayer; // 释放无效图层
        return;
    }

    // 如果没有有效的 CRS，手动设置一个默认 CRS（例如 EPSG:4326）
    if (!vectorLayer->crs().isValid()) {
        QgsCoordinateReferenceSystem crs("EPSG:4326"); // 设置默认 CRS
        vectorLayer->setCrs(crs); // 设置 CRS
        qDebug() << "CRS was invalid. Setting to EPSG:4326";
    }

    // 输出 CRS 信息
    QgsCoordinateReferenceSystem crs = vectorLayer->crs();
    qDebug() << "CRS of vector layer: " << crs.authid();  // 输出 CRS

    // 加载图层并更新字段下拉框
    m_vectorLayer = vectorLayer; // 将当前图层赋值给 m_vectorLayer

    ui.fieldComboBox->clear(); // 清空下拉框
    QgsFields fields = vectorLayer->fields();
    for (int i = 0; i < fields.count(); ++i) {
        ui.fieldComboBox->addItem(fields.at(i).name());
    }
}

//保存栅格文件路径
void Rasterize::on_browseRasterFileButton_clicked()
{
    m_outputFilePath = QFileDialog::getSaveFileName(this, QStringLiteral("选择保存路径"), "", "GeoTIFF Files (*.tif)");
    if (m_outputFilePath.isEmpty())
        return;

    ui.rasterFileLineEdit->setText(m_outputFilePath);
}

void Rasterize::on_convertToRasterButton_clicked()
{
    // 获取选中的索引
    int currentIndex = ui.fileComboBox->currentIndex();

    if (currentIndex < 0 || currentIndex >= m_vectorLayers.size()) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("矢量图层不存在！"));
        return;
    }

    // 基于当前索引获取选中的图层
    m_vectorLayer = m_vectorLayers[currentIndex];

    // 确保加载了有效的矢量图层
    if (!m_vectorLayer || !m_vectorLayer->isValid()) {
        QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("矢量图层无效！"));
        return;
    }

    // 获取矢量图层的坐标参考系统
    QgsCoordinateReferenceSystem crs = m_vectorLayer->crs();

    // 获取输出文件路径
    QString outputFilePath = ui.rasterFileLineEdit->text();
    if (outputFilePath.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Error"), QStringLiteral("没有输出路径！"));
        return;
    }

    // 获取矢量图层范围（EXTENT）
    QgsRectangle extent = m_vectorLayer->extent();

    // 如果需要，可以进行坐标参考系统转换
    QgsCoordinateReferenceSystem targetCRS = QgsCoordinateReferenceSystem("EPSG:4326");  // 目标 CRS（例如 WGS 84）
    if (crs != targetCRS) {
        QgsCoordinateTransform transform(crs, targetCRS, QgsProject::instance());
        extent = transform.transform(extent);  // 转换范围到目标 CRS
    }

    QString extentString = QString("%1,%2,%3,%4")
        .arg(extent.xMinimum())
        .arg(extent.xMaximum())
        .arg(extent.yMinimum())
        .arg(extent.yMaximum());

    // 获取用户输入的像元大小（从lineEdit获取）
    bool isok;
    double pixelSize = ui.pixelSizeLineEdit->text().toDouble(&isok);  // 假设lineEdit的对象名为pixelSizeLineEdit
    if (!isok || pixelSize <= 0) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("像元大小无效！"));
        return;
    }


    int tileSize = qMin(1024, std::max(64, static_cast<int>(qSqrt(extent.width() * extent.height()) / 2)));

    // 设置栅格化参数
    QVariantMap parameters;
    parameters.insert("INPUT", m_vectorLayer->source());
    parameters.insert("FIELD", ui.fieldComboBox->currentText());
    parameters.insert("EXTENT", extentString);                             // 栅格化范围
    parameters.insert("MAP_UNITS_PER_PIXEL", pixelSize);                   // 每像素的地图单位
    parameters.insert("OUTPUT", outputFilePath);
    parameters.insert("TILE_SIZE", tileSize);                                // 动态计算的图块大小


    // 获取栅格化算法
    QgsProcessingAlgorithm* alg = QgsApplication::processingRegistry()->createAlgorithmById("native:rasterize");
    if (!alg) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("无法加载栅格化算法！"));
        return;
    }

    // 创建处理上下文
    QgsProcessingContext context;
    QgsProject* project = QgsProject::instance();
    project->setCrs(crs);  // 使用矢量图层的 CRS
    project->addMapLayer(m_vectorLayer);
    context.setProject(project);

    QgsProcessingFeedback feedback;

    // 准备并运行算法
    if (!alg->prepare(parameters, context, &feedback)) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("算法运行失败！"));
        delete alg;
        return;
    }

    bool ok = false;
    QVariantMap results = alg->run(parameters, context, &feedback, &ok);

    // 检查执行结果
    if (!ok) {
        QString feedbackLog = feedback.textLog();
        QMessageBox::critical(this, tr("Error"), QStringLiteral("栅格化失败 Log:\n") + feedbackLog);
        delete alg;
        return;
    }

    // 检查输出文件是否存在
    if (!QFile::exists(outputFilePath)) {
        QMessageBox::critical(this, tr("Error"), QStringLiteral("保存至指定路径失败！"));
        delete alg;
        return;
    }


    QFileInfo fi(m_outputFilePath);
    if (!fi.exists()) { return; }
    QString layerBaseName = fi.baseName(); // 图层名称

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
