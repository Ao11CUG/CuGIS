#include "MainInterface.h"
#include "qgsapplication.h"
#include <QFileDialog>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QgsMapCanvas.h>
#include <QgsVectorLayer.h>
#include <QgsRasterLayer.h>
#include <QgsProject.h>
#include <QgsLayerTreeMapCanvasBridge.h>
#include <QMessageBox>
#include <QgsProject.h>
#include <QgsApplication.h>
#include <QMenu>
#include <QAction>
#include <qgsattributetablemodel.h>
#include <QDialog>
#include <qgsattributetablefiltermodel.h>
#include <qgsvectorlayercache.h>
#include "qgsattributetableview.h"
#include "qgssourcefieldsproperties.h"
#include "qgssymbol.h"
#include "qgslayertreenode.h"
#include <QMouseEvent>
#include "qgspolygon.h"
#include "qgspointxy.h"
#include "QShortcut"
#include "DrawGeometryTool.h"
#include <QgsSingleSymbolRenderer.h>
#include <QInputDialog>
#include <QgsCoordinateReferenceSystem.h>
#include <QgsProcessingAlgorithm.h>
#include <QgsProcessingContext.h>
#include <QgsProcessingFeedback.h>
#include <QgsProcessingRegistry.h>
#include <qgsnativealgorithms.h>

MainInterface::MainInterface(QWidget* parent)
    : QMainWindow(parent)
    , symbolLibrary(new SymbolLibManager(this))
{
    //初始化
    m_mapCanvas = nullptr;
    m_layerTreeView = nullptr;
    m_rubberBand = nullptr;

    ui.setupUi(this);
    setWindowIcon(QIcon(":/MainInterface/res/cug.png"));

    setStyle();

    // 全屏显示
    showFullScreen();
    setWindowState(Qt::WindowMaximized);

    // 初始化QGIS应用程序
    QgsApplication::init();
    QgsApplication::initQgis();

    // 初始化地图画布
    m_mapCanvas = new QgsMapCanvas(this);
    m_mapCanvas->setCanvasColor(Qt::white);  // 设置背景颜色
    ui.scrollArea->setWidget(m_mapCanvas);

    // 设置目标CRS，QGIS会自动处理动态投影和叠加显示
    QgsCoordinateReferenceSystem targetCRS("EPSG:4326"); 
    m_mapCanvas->setDestinationCrs(targetCRS);

    // 初始化图层管理器
    m_layerTreeView = new QgsLayerTreeView();
    initLayerTreeView();
    connect(m_layerTreeView, &QgsLayerTreeView::customContextMenuRequested, this, &MainInterface::onTreeViewContextMenu);

    // 初始化鹰眼图
    m_eagleEyeWidget = new EagleEye(m_mapCanvas);
    initEagleEye(m_mapCanvas);

    // 初始化工具箱
    initToolBox();
    QgsApplication::processingRegistry()->addProvider(new QgsNativeAlgorithms());   //初始化算法
    ui.selectFeatureAct->setEnabled(isEditing);
    ui.deleteFeatureAct->setEnabled(isEditing);
    ui.moveFeatureAct->setEnabled(isEditing);
    ui.copyFeatureAct->setEnabled(isEditing);
    ui.viewAttributeAct->setEnabled(isEditing);

    connect(ui.undoAct, &QAction::triggered, this, &MainInterface::undo); // 连接到撤销函数
    connect(ui.redoAct, &QAction::triggered, this, &MainInterface::redo); // 连接到撤销函数
}

MainInterface::~MainInterface()
{}

// 设置样式
void MainInterface::setStyle() {
    style.setMenuBar(ui.menuBar);
}

// 设置坐标系
void MainInterface::on_setCrsAct_triggered() {
    // 创建一个对话框
    QDialog dialog(this);
    dialog.resize(600, 200);
    dialog.setWindowTitle(QStringLiteral("选择目标坐标系"));

    // 创建布局
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // 创建 QComboBox
    QComboBox* crsComboBox = new QComboBox(&dialog);
    style.setComboBox(crsComboBox);
    layout->addWidget(crsComboBox);

    // 添加常用的 EPSG 坐标系
    crsComboBox->addItem("EPSG:4326 (WGS 84)", "EPSG:4326");
    crsComboBox->addItem("EPSG:3857 (Web Mercator)", "EPSG:3857");
    crsComboBox->addItem("EPSG:27700 (OSGB 1936)", "EPSG:27700");
    // 可以根据需要添加更多坐标系

    // 创建确认按钮
    QPushButton* okButton = new QPushButton(QStringLiteral("确定"), &dialog);
    style.setConfirmButton(okButton);
    layout->addWidget(okButton);

    // 连接按钮点击信号
    connect(okButton, &QPushButton::clicked, [&]() {
        // 获取选择的 EPSG 代码
        QString epsgCode = crsComboBox->currentData().toString();
        QgsCoordinateReferenceSystem targetCRS(epsgCode);

        // 设置地图画布的目标 CRS
        m_mapCanvas->setDestinationCrs(targetCRS);

        dialog.accept(); // 关闭对话框
        });

    // 显示对话框
    dialog.exec(); // 显示选择窗口并等待用户操作
}

// 打开矢量文件
void MainInterface::on_openVectorFileAct_triggered()
{
    QStringList layerPathList = QFileDialog::getOpenFileNames(this, QStringLiteral("选择矢量文件"), "", "Shapefiles (*.shp)");
    QList<QgsMapLayer*> layerList;
    for each (QString layerPath in layerPathList)
    {
        QFileInfo fi(layerPath);
        if (!fi.exists()) { return; }
        QString layerBaseName = fi.baseName(); // 图层名称

        QgsVectorLayer* vecLayer = new QgsVectorLayer(layerPath, layerBaseName);
        if (!vecLayer) { return; }
        if (!vecLayer->isValid())
        {
            QMessageBox::information(0, "", QStringLiteral("无效图层！"));
            return;
        }
        layerList << vecLayer;

        // 存储该图层的初始范围
        m_layerExtents[vecLayer] = vecLayer->extent();
    }
    if (layerList.size() < 1)
        return;

    //选择图层
    connect(m_layerTreeView, &QgsLayerTreeView::currentLayerChanged, this, &MainInterface::onCurrentLayerChanged);

    m_curMapLayer = layerList[0];

    QgsProject::instance()->addMapLayers(layerList);

    // 设置画布视图范围为该图层的初始范围
    m_mapCanvas->setExtent(m_layerExtents[m_curMapLayer]);

    m_mapCanvas->refresh();

    // 刷新鹰眼图
    m_eagleEyeWidget->updateEagleEye();
    m_eagleEyeWidget->eagleEyeCanvas->refresh();
}

// 打开栅格文件
void MainInterface::on_openRasterFileAct_triggered()
{
    QStringList layerPathList = QFileDialog::getOpenFileNames(this, QStringLiteral("选择栅格文件"), "", "GeoTIFF Files (*.tif *.tiff)");
    QList<QgsMapLayer*> layerList;
    for each (QString layerPath in layerPathList)
    {
        QFileInfo fi(layerPath);
        if (!fi.exists()) { return; }
        QString layerBaseName = fi.baseName(); // 图层名称

        QgsRasterLayer* rasterLayer = new QgsRasterLayer(layerPath, layerBaseName);
        if (!rasterLayer) { return; }
        if (!rasterLayer->isValid())
        {
            QMessageBox::information(0, "", QStringLiteral("无效图层！"));
            return;
        }
        layerList << rasterLayer;

        // 存储该图层的初始范围
        m_layerExtents[rasterLayer] = rasterLayer->extent();
    }
    m_curMapLayer = layerList[0];

    QgsProject::instance()->addMapLayers(layerList);
    // 设置画布视图范围为该图层的初始范围
    m_mapCanvas->setExtent(m_layerExtents[m_curMapLayer]);
    m_mapCanvas->refresh();

    // 刷新鹰眼图
    m_eagleEyeWidget->updateEagleEye();
    m_eagleEyeWidget->eagleEyeCanvas->refresh();
}

// 打开工程文件
void MainInterface::on_openProjectFileAct_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, QStringLiteral("选择工程文件"), "", "QGIS project (*.qgs)");
    QFileInfo fi(filename);
    if (!fi.exists())
    {
        return;
    }

    QgsProject::instance()->read(filename);

    // 刷新鹰眼图
    m_eagleEyeWidget->updateEagleEye();
    m_eagleEyeWidget->eagleEyeCanvas->refresh();
}

// 保存文件逻辑
void MainInterface::on_saveFileAct_triggered()
{
    QStringList layerFileNames; // 存放图层文件名
    const QMap<QString, QgsMapLayer*>& layers = QgsProject::instance()->mapLayers();

    // 收集所有图层文件名
    for (const auto& layerPair : layers)
    {
        QgsMapLayer* layer = layerPair; // 直接获取层指针
        if (layer)
        {
            layerFileNames.append(layer->dataProvider()->dataSourceUri()); // 收集数据源 URI
        }
    }

    bool hasShp = false;
    bool hasTif = false;

    // 检查文件扩展名
    for (const QString& fileName : layerFileNames)
    {
        if (fileName.endsWith(".shp", Qt::CaseInsensitive)) // 矢量文件
        {
            hasShp = true; // 设置标志
        }
        else if (fileName.endsWith(".tif", Qt::CaseInsensitive) || fileName.endsWith(".tiff", Qt::CaseInsensitive)) // 栅格文件
        {
            hasTif = true; // 设置标志
        }
    }

    QString saveFilename;
    if (hasShp && !hasTif) // 只有 shp 图层时保存为 shp
    {
        saveFilename = QFileDialog::getSaveFileName(this, QStringLiteral("保存文件"), "", "shapefile (*.shp)");
        // 调用保存逻辑
    }
    else if (hasTif && !hasShp) // 只有 tif 图层时保存为 tif
    {
        saveFilename = QFileDialog::getSaveFileName(this, QStringLiteral("保存文件"), "", "Image (*.tif)");
        // 调用保存逻辑
    }
    else if (hasShp && hasTif) // 都存在时保存为 qgz
    {
        saveFilename = QFileDialog::getSaveFileName(this, QStringLiteral("保存文件"), "", "QGIS project (*.qgz)");
        // 调用保存逻辑
    }

    if (!saveFilename.isEmpty())
    {
        QgsProject::instance()->write(saveFilename); // 保存项目
    }
}

// 移除图层
void MainInterface::removeLayer()
{
    if (!m_layerTreeView) { return; }

    QModelIndexList indexes;
    while ((indexes = m_layerTreeView->selectionModel()->selectedRows()).size()) {
        QgsMapLayer *layer = m_layerTreeView->currentLayer();
        if (layer) {
            // 从 QGIS 项目中移除图层
            QgsProject::instance()->removeMapLayer(layer->id());
            m_mapCanvas->refresh();

            // 刷新鹰眼图
            m_eagleEyeWidget->updateEagleEye();
            m_eagleEyeWidget->eagleEyeCanvas->refresh();
        }
    }
}

// 全屏显示
void MainInterface::on_fullScreenAct_triggered() {
    if (m_mapCanvas) {
        // 创建一个新的 QDialog 对象
        QDialog* fullScreenDialog = new QDialog(this);
        fullScreenDialog->setWindowIcon(QIcon(":/MainInterface/res/cug.png"));
        fullScreenDialog->setWindowTitle(QStringLiteral("全屏显示"));

        // 创建一个新的 QGIS MapCanvas 实例
        QgsMapCanvas* fullScreenMapCanvas = new QgsMapCanvas(fullScreenDialog);

        // 设置与原始 MapCanvas 相同的图层、缩放和中心位置
        fullScreenMapCanvas->setLayers(m_mapCanvas->layers());
        fullScreenMapCanvas->setExtent(m_mapCanvas->extent());
        fullScreenMapCanvas->setCenter(m_mapCanvas->center());

        // 设置布局
        QVBoxLayout* layout = new QVBoxLayout(fullScreenDialog);

        // 添加 Map Canvas 到对话框
        layout->addWidget(fullScreenMapCanvas);

        fullScreenDialog->setLayout(layout);

        // 设置对话框为全屏
        fullScreenDialog->showFullScreen();
        fullScreenDialog->setWindowState(Qt::WindowMaximized);
    }
}

// 初始化鹰眼图
void MainInterface::initEagleEye(QgsMapCanvas* mainCanvas) {
    ui.LeftDockDown->setWidget(m_eagleEyeWidget);
}

// 打开鹰眼图
void MainInterface::on_openEagleEyeAct_triggered()
{
    ui.LeftDockDown->setVisible(!ui.LeftDockDown->isVisible());
}

// 初始化图层管理器
void MainInterface::initLayerTreeView()
{
    QgsLayerTreeModel* model = new QgsLayerTreeModel(QgsProject::instance()->layerTreeRoot(), this);
    qDebug() << QgsProject::instance()->layerTreeRoot();
    model->setFlag(QgsLayerTreeModel::AllowNodeReorder);
    model->setFlag(QgsLayerTreeModel::AllowNodeRename);
    model->setFlag(QgsLayerTreeModel::AllowNodeChangeVisibility);
    model->setFlag(QgsLayerTreeModel::ShowLegendAsTree);
    model->setFlag(QgsLayerTreeModel::UseEmbeddedWidgets);
    model->setAutoCollapseLegendNodes(10);
    m_layerTreeView->setModel(model);
    m_layerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge(QgsProject::instance()->layerTreeRoot(), m_mapCanvas, this);
    //支持右键菜单
    m_layerTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(QgsProject::instance(), SIGNAL(writeProject(QDomDocument&)), m_layerTreeCanvasBridge, SLOT(writeProject(QDomDocument&)));
    connect(QgsProject::instance(), SIGNAL(readProject(QDomDocument)), m_layerTreeCanvasBridge, SLOT(readProject(QDomDocument)));
    // 添加组命令
    QAction* actionAddGroup = new QAction(QStringLiteral("+"), this);
    actionAddGroup->setToolTip(QStringLiteral("添加组"));
    connect(actionAddGroup, &QAction::triggered, m_layerTreeView->defaultActions(), &QgsLayerTreeViewDefaultActions::addGroup);

    // 双击事件，打开图层样式编辑器
    connect(m_layerTreeView, &QgsLayerTreeView::doubleClicked, this, &MainInterface::onLayerTreeItemDoubleClicked);

    // 扩展和收缩图层树
    QAction* actionExpandAll = new QAction(QStringLiteral("≡"), this);
    actionExpandAll->setToolTip(QStringLiteral("展开所有组"));
    connect(actionExpandAll, &QAction::triggered, m_layerTreeView, &QgsLayerTreeView::expandAllNodes);
    QAction* actionCollapseAll = new QAction(QStringLiteral("—"), this);
    actionCollapseAll->setToolTip(QStringLiteral("折叠所有组"));
    connect(actionCollapseAll, &QAction::triggered, m_layerTreeView, &QgsLayerTreeView::collapseAllNodes);

    // 移除图层
    QAction* actionRemoveLayer = new QAction(QStringLiteral("×"));
    actionRemoveLayer->setToolTip(QStringLiteral("移除图层/组"));
    connect(actionRemoveLayer, &QAction::triggered, this, &MainInterface::removeLayer);

    QToolBar* toolbar = new QToolBar();
    toolbar->setIconSize(QSize(30, 20));
    toolbar->addAction(actionAddGroup);
    toolbar->addAction(actionExpandAll);
    toolbar->addAction(actionCollapseAll);
    toolbar->addAction(actionRemoveLayer);

    QVBoxLayout* vBoxLayout = new QVBoxLayout();
    vBoxLayout->setMargin(0);
    vBoxLayout->setContentsMargins(0, 0, 0, 0);
    vBoxLayout->setSpacing(0);
    vBoxLayout->addWidget(toolbar);
    vBoxLayout->addWidget(m_layerTreeView);

    QWidget* w = new QWidget;
    w->setLayout(vBoxLayout);
    this->ui.LeftDockUp->setWidget(w);
}

// 打开图层管理器
void MainInterface::on_openLayerTreeAct_triggered()
{
    ui.LeftDockUp->setVisible(!ui.LeftDockUp->isVisible());
}

// 初始化工具箱
void MainInterface::initToolBox()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 创建工具箱实例
    QStringList pointContents = { QStringLiteral("添加点"), QStringLiteral("光滑线"), QStringLiteral("延长线"), QStringLiteral("旋转线"), QStringLiteral("简化线") ,QStringLiteral("合并面"), QStringLiteral("分割面"), QStringLiteral("边界提取") };
    QStringList pointIcons = { ":/MainInterface/res/point.png", ":/MainInterface/res/line.png",  ":/MainInterface/res/line.png" ,":/MainInterface/res/line.png", ":/MainInterface/res/line.png", ":/MainInterface/res/surface.png",":/MainInterface/res/surface.png",":/MainInterface/res/surface.png" };
    ToolBox* pointToolbox = new ToolBox(QStringLiteral("矢量编辑"), pointContents, pointIcons, this);
    mainLayout->addWidget(pointToolbox);

    // 提取按钮并连接信号到槽
    QList<QPushButton*> pointButtons = pointToolbox->getButtons();
    connect(pointButtons[0], &QPushButton::clicked, this, &MainInterface::on_addPointAct_triggered);
    connect(pointButtons[1], &QPushButton::clicked, this, &MainInterface::on_smoothGeometryAct_triggered);
    connect(pointButtons[2], &QPushButton::clicked, this, &MainInterface::on_extentLineAct_triggered);
    connect(pointButtons[3], &QPushButton::clicked, this, &MainInterface::on_rotateLineAct_triggered);
    connect(pointButtons[4], &QPushButton::clicked, this, &MainInterface::on_simplifyLineAct_triggered);
    connect(pointButtons[5], &QPushButton::clicked, this, &MainInterface::on_faceMergeAct_triggered);
    connect(pointButtons[6], &QPushButton::clicked, this, &MainInterface::on_divideSurfaceAct_triggered);
    connect(pointButtons[7], &QPushButton::clicked, this, &MainInterface::on_extractBoundaryAct_triggered);

    QStringList analysisContents = { QStringLiteral("矢量缓冲区分析"), QStringLiteral("矢量裁剪分析"), QStringLiteral("矢量转栅格"), QStringLiteral("栅格计算器") , QStringLiteral("表格转矢量") };
    QStringList analysisIcons = { ":/MainInterface/res/bufferAnalysis.png", ":/MainInterface/res/vectorTailor.png", ":/MainInterface/res/rasterize.png",  ":/MainInterface/res/rasterCalculator.png", ":/MainInterface/res/excelToVector.png" };
    ToolBox* analysisToolbox = new ToolBox(QStringLiteral("空间分析"), analysisContents, analysisIcons, this);
    mainLayout->addWidget(analysisToolbox);

    // 提取按钮并连接信号到槽
    QList<QPushButton*> analysisButtons = analysisToolbox->getButtons();
    connect(analysisButtons[0], &QPushButton::clicked, this, &MainInterface::on_bufferAnalysisAct_triggered);
    connect(analysisButtons[1], &QPushButton::clicked, this, &MainInterface::on_vectorTailorAct_triggered);
    connect(analysisButtons[2], &QPushButton::clicked, this, &MainInterface::on_rasterizeAct_triggered);
    connect(analysisButtons[3], &QPushButton::clicked, this, &MainInterface::on_rasterCalculatorAct_triggered);
    connect(analysisButtons[4], &QPushButton::clicked, this, &MainInterface::on_csvToVectorAct_triggered);

    // 创建一个 QWidget 并设置布局
    QWidget* container = new QWidget();
    container->setLayout(mainLayout);

    ui.RightDock->setWidget(container);
}

// 打开工具箱
void MainInterface::on_openToolBoxAct_triggered()
{
    ui.RightDock->setVisible(!ui.RightDock->isVisible());
}

// 矢量转栅格
void MainInterface::on_rasterizeAct_triggered() {
    Rasterize* rasterize = new Rasterize(m_mapCanvas, this);
    rasterize->show();
}

// 栅格计算器
void MainInterface::on_rasterCalculatorAct_triggered() {
    RasterCalculator* rastercalc = new RasterCalculator(m_mapCanvas, this);
    rastercalc->show();
}

// 矢量裁剪分析
void MainInterface::on_vectorTailorAct_triggered() {
    VectorClippingFunction* vectorClip = new VectorClippingFunction(m_mapCanvas, this);
    vectorClip->show();
}

// 矢量缓冲区分析
void MainInterface::on_bufferAnalysisAct_triggered() {
    BufferAnalysisFunction* bufferAnalysis = new BufferAnalysisFunction(m_mapCanvas, this);
    bufferAnalysis->show();
}

// 表格转矢量
void MainInterface::on_csvToVectorAct_triggered() {
    //调用csvReader里面的函数来实现
    QgsVectorLayer* vectorLayer = m_csvReader->openCSVFile();
    if (vectorLayer == nullptr)
    {
        return;
    }
    m_mapCanvas->setLayers({});
    QgsProject::instance()->clear();
    QgsProject::instance()->addMapLayer(vectorLayer);
    m_mapCanvas->setLayers({ vectorLayer });
    m_mapCanvas->setExtent(vectorLayer->extent());
    m_mapCanvas->refresh();

}

// 树图层的选择小窗
void MainInterface::onTreeViewContextMenu(const QPoint& pos) {
    // 获取鼠标右键点击的项(重新创建指向图层的指针)
    QgsMapLayer* mapLayer = m_layerTreeView->currentLayer();
    // 创建右键菜单
    QMenu contextMenu(this);
    // 添加菜单项
    QAction* actionOpenWindow = contextMenu.addAction(QStringLiteral("查看"));

    // 检查图层是否为矢量图层
    QAction* actionOpenAttribute = nullptr;
    QAction* actionOpenAttributeStruction = nullptr;

    if (mapLayer->type() == Qgis::LayerType::Vector) {
        actionOpenAttribute = contextMenu.addAction(QStringLiteral("打开属性表"));
        actionOpenAttributeStruction = contextMenu.addAction(QStringLiteral("打开属性结构表"));
    }

    // 显示菜单并捕获用户选择的动作
    QAction* selectedAction = contextMenu.exec(m_layerTreeView->viewport()->mapToGlobal(pos));
    if (selectedAction == actionOpenWindow) {
        // 如果选择了“打开窗口”，执行对应操作
        m_attribute->openLayerInfoWindow(mapLayer, this);
    }
    else if (selectedAction == actionOpenAttribute) {
        // 如果选择了“打开属性表”，执行对应操作
        QgsVectorLayer* vectorLayer = dynamic_cast<QgsVectorLayer*>(mapLayer);
        m_attribute->showAttributeTable(vectorLayer, this);
    }
    else if (selectedAction == actionOpenAttributeStruction) {
        // 如果选择了“打开属性表”，执行对应操作
        QgsVectorLayer* vectorLayer = dynamic_cast<QgsVectorLayer*>(mapLayer);
        m_attribute->openAttributeEditor(vectorLayer, this);
    }
}

// 处理编辑模式
void MainInterface::toggleLayerEditing(QgsVectorLayer* layer)
{
    // 判断当前图层是否已经处于编辑状态
    if (layer->isEditable()) {
        // 如果已经在编辑状态，结束编辑
        if (layer->commitChanges()) {
            QMessageBox::information(this, QStringLiteral("图层"), QStringLiteral("结束编辑模式"));
            isEditing = false; // 标记图层为非编辑状态
            m_isSelecting = false;
            m_currentMode = Mode::None;

            selectedFeatureIds.clear(); // 清空选择的要素ID
            m_selectedFeature = QgsFeature(); // 重置当前选中的要素

            // 清理之前的地图工具
            if (identifyTool) {
                // 假设有一个指向当前地图工具的指针
                m_mapCanvas->setMapTool(nullptr); // 取消当前地图工具的激活
                identifyTool = nullptr; // 清空当前地图工具
            }
        }
        else {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("提交更改失败！"));
        }
    }
    else {
        // 启动编辑模式
        if (layer->startEditing()) {
            QMessageBox::information(this, QStringLiteral("图层"), QStringLiteral("开启编辑模式"));
            isEditing = true; // 标记图层为编辑状态
        }
        else {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("启用编辑模式失败！"));
        }
    }

    // 启用/禁用与编辑相关的按钮
    ui.addPointAct->setEnabled(isEditing);
    ui.selectFeatureAct->setEnabled(isEditing);
    ui.deleteFeatureAct->setEnabled(isEditing);
    ui.moveFeatureAct->setEnabled(isEditing);
    ui.copyFeatureAct->setEnabled(isEditing);
    ui.viewAttributeAct->setEnabled(isEditing);
}

// 开始编辑按钮
void MainInterface::on_startEditingAct_triggered()
{
    // 查找第一个矢量图层
    QgsVectorLayer* layer = nullptr;
    for (QgsMapLayer* mapLayer : m_mapCanvas->layers()) {
        if (mapLayer->type() == Qgis::LayerType::Vector) {
            layer = qobject_cast<QgsVectorLayer*>(mapLayer); // 使用 qobject_cast 安全转换类型
            break; // 找到第一个矢量图层，跳出循环
        }
    }

    // 如果没有找到矢量图层，弹出提示
    if (layer == nullptr) {
        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("找不到矢量图层！"));
        return;
    }

    // 结束编辑模式
    toggleLayerEditing(layer);
}

// 退出编辑模式
void MainInterface::on_exitAct_triggered()
{
    // 查找第一个矢量图层
    QgsVectorLayer* layer = nullptr;
    for (QgsMapLayer* mapLayer : m_mapCanvas->layers()) {
        if (mapLayer->type() == Qgis::LayerType::Vector) {
            layer = qobject_cast<QgsVectorLayer*>(mapLayer); // 使用 qobject_cast 安全转换类型
            break; // 找到第一个矢量图层，跳出循环
        }
    }

    // 如果没有找到矢量图层，弹出提示
    if (layer == nullptr) {
        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("找不到矢量图层！"));
        return;
    }

    // 结束编辑模式
    toggleLayerEditing(layer);
    }

// 添加点按钮
void MainInterface::on_addPointAct_triggered()
{
    m_currentMode = Mode::Add;
    // 弹出选择类型的对话框
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/MainInterface/res/cug.png"));
    msgBox.setWindowTitle(QStringLiteral("选择"));
    msgBox.setText(QStringLiteral("输入类型"));
    QPushButton* pointButton = msgBox.addButton(QStringLiteral("点要素"), QMessageBox::AcceptRole);
    style.setConfirmButton(pointButton);
    QPushButton* labelButton = msgBox.addButton(QStringLiteral("注记"), QMessageBox::RejectRole);
    style.setConfirmButton(labelButton);
    msgBox.exec();

    // 判断用户的选择
    if (msgBox.clickedButton() == pointButton) {
        // 如果选择了点要素，使用点绘制工具
        if (!isEditing) {
            return;
        }

        // 创建 QGIS 绘制工具用于点要素输入
        QgsMapToolEmitPoint* tool = new QgsMapToolEmitPoint(m_mapCanvas);
        connect(tool, &QgsMapToolEmitPoint::canvasClicked, this, &MainInterface::onCanvasReleased);
        m_mapCanvas->setMapTool(tool);

    }
    else if (msgBox.clickedButton() == labelButton) {
        if (!isEditing) {
            //  QMessageBox::warning(this, "Error", "Layer is not in editing mode.");
            return;
        }
        QgsMapToolEmitPoint* texttool = new QgsMapToolEmitPoint(m_mapCanvas);
        connect(texttool, &QgsMapToolEmitPoint::canvasClicked, this, &MainInterface::textLabel);
        m_mapCanvas->setMapTool(texttool);
    }

}

// 查看属性动作
void MainInterface::on_viewAttributeAct_triggered() {
    if (m_isSelecting == 1) {
        m_currentMode = Mode::View;

        // 创建 QGIS 绘制工具用于点要素输入
        QgsMapToolEmitPoint* tool = new QgsMapToolEmitPoint(m_mapCanvas);
        connect(tool, &QgsMapToolEmitPoint::canvasClicked, this, &MainInterface::onCanvasReleased);
        m_mapCanvas->setMapTool(tool);
    }
    else QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("请先进入选择模式！"));
}

// 添加点鼠标响应的槽函数
void MainInterface::onCanvasReleased(const QgsPointXY& point, Qt::MouseButton button)
{
    Mode mode = m_currentMode;

    // 判断是否是左键点击
    if (button != Qt::LeftButton) {
        return; // 如果不是左键点击，则直接返回
    }

    // 查找第一个矢量图层
    m_layer = nullptr;
    for (QgsMapLayer* mapLayer : m_mapCanvas->layers()) {
        if (mapLayer->type() == Qgis::LayerType::Vector) {
            m_layer = qobject_cast<QgsVectorLayer*>(mapLayer); // 使用 qobject_cast 安全转换类型
            break; // 找到第一个矢量图层，跳出循环
        }
    }
    // 如果没有找到矢量图层
    if (m_layer == nullptr) {
        return;
    }

    // 获取图层的字段列表
    QgsFields fields = m_layer->fields();
    QList<QgsFeature> selectedFeatures = m_layer->selectedFeatures();

    if (m_currentMode == Mode::Add) {
        // 检查图层的几何类型是否为点
        if (m_layer->geometryType() == Qgis::GeometryType::Point) {
            // 启动编辑
            if (!m_layer->isEditable()) {
                if (!m_layer->startEditing()) {
                    return;
                }
            }

            // 获取点击位置的坐标
            QgsPointXY mapPoint = point;

            // 创建临时高亮点
            QgsRubberBand* rubberBand = new QgsRubberBand(m_mapCanvas, Qgis::GeometryType::Point);
            rubberBand->setColor(Qt::red);  // 设置高亮点颜色
            rubberBand->setWidth(7);        // 设置宽度
            rubberBand->addPoint(mapPoint); // 在点击位置添加点

            // 创建对话框
            QDialog dialog(this);
            dialog.resize(600, 200);
            dialog.setWindowIcon(QIcon(":/MainInterface/res/cug.png"));
            dialog.setWindowTitle(QStringLiteral("输入属性要素"));

            QFormLayout* formLayout = new QFormLayout(&dialog);

            // 动态添加输入框
            QList<QLineEdit*> lineEdits;
            for (int i = 0; i < fields.count(); ++i) {
                const QgsField& field = fields[i];
                QLineEdit* lineEdit = new QLineEdit(&dialog);
                style.setLineEdit(lineEdit);
                lineEdits.append(lineEdit);
                formLayout->addRow(field.name(), lineEdit);
            }

            // 创建 OK 和 Cancel 按钮
            QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
            style.setDialogConfirmButton(buttonBox);
            formLayout->addWidget(buttonBox);

            // 连接按钮信号槽
            connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

            // 显示对话框并等待用户输入
            if (dialog.exec() != QDialog::Accepted) {
                // 如果用户点击了取消，则删除高亮的点，并返回
                delete rubberBand;
                return;
            }

            // 获取用户输入的属性值
            QList<QVariant> dataList;  // 你的 QVariant 列表
            for (QLineEdit* lineEdit : lineEdits) {
                dataList.append(lineEdit->text());
            }

            // 需要一个 QgsAttributes 对象来存储这些数据
            QgsAttributes attributes;

            // 遍历 QVariant 列表，并将其添加到 QgsAttributes 中
            for (int i = 0; i < dataList.size(); ++i) {
                // 创建一个 QgsField 对象，可以设置合适的字段类型
                QgsField field(QString("field_%1").arg(i), QVariant::Type(dataList[i].type()));
                attributes.append(dataList[i]);
            }

            // 如果字段数量不匹配，弹出提示
            if (dataList.size() != fields.count()) {
                QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("输入的属性数量与图层字段数量不匹配！"));
                delete rubberBand;
                return;
            }

            QList<QgsFeature> newFeatures;

            // 创建一个新的要素
            QgsFeature feature;
            feature.setGeometry(QgsGeometry::fromPointXY(mapPoint));
            feature.setAttributes(attributes);

            newFeatures.append(feature);

            // 获取图层的数据提供器并添加要素
            QgsVectorDataProvider* provider = m_layer->dataProvider();
            provider->addFeature(feature);

            // 在添加要素后记录撤销操作
            undoStack.append(UndoAction(ADD, newFeatures)); // 历史记录为加要素

            // 更新图层显示
            m_layer->triggerRepaint();

            // 删除临时高亮点
            delete rubberBand;
        }
        else if (m_layer->geometryType() == Qgis::GeometryType::Line || m_layer->geometryType() == Qgis::GeometryType::Polygon) {
            // 创建新的字段列表
            QList<QgsField> fields;
            fields.append(QgsField("id", QVariant::Int));
            fields.append(QgsField("name", QVariant::String));

            // 创建空内存图层
            QgsVectorLayer* newLayer = new QgsVectorLayer("Point?crs=EPSG:4326", "New Point", "memory");
            if (!newLayer->isValid()) {
                qWarning() << "Layer failed to create!";
                delete newLayer; // 确保释放内存
                return;
            }

            // 添加字段到图层
            QgsVectorDataProvider* provider = newLayer->dataProvider();
            provider->addAttributes(fields); // 增加字段
            newLayer->updateFields(); // 更新图层字段

            // 启动编辑模式
            if (!newLayer->isEditable()) {
                if (!newLayer->startEditing()) {
                    return;
                }
            }

            // 获取点击位置的坐标
            QgsPointXY mapPoint = point;

            // 创建一个新的要素
            QgsFeature feature;
            feature.setGeometry(QgsGeometry::fromPointXY(mapPoint));

            // 创建对话框以获取用户输入的属性值
            QDialog dialog(this);
            dialog.resize(600, 200);
            dialog.setWindowTitle(QStringLiteral("输入属性要素"));

            QFormLayout* formLayout = new QFormLayout(&dialog);
            QList<QLineEdit*> lineEdits;

            // 动态添加输入框
            for (const QgsField& field : fields) {
                QLineEdit* lineEdit = new QLineEdit(&dialog);
                lineEdits.append(lineEdit);
                style.setLineEdit(lineEdit);
                formLayout->addRow(field.name(), lineEdit);
            }

            // 创建 OK 和 Cancel 按钮
            QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
            style.setDialogConfirmButton(buttonBox);
            formLayout->addWidget(buttonBox);

            // 连接按钮信号槽
            connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

            // 显示对话框并等待用户输入
            if (dialog.exec() != QDialog::Accepted) {
                delete newLayer; // 如果用户点击取消，删除图层
                return;
            }

            // 获取用户输入的属性值并设置到要素中
            QgsAttributes attributes;

            for (const QLineEdit* lineEdit : lineEdits) {
                attributes.append(lineEdit->text());  // 使用用户输入的文本填充属性
            }

            // 设置要素的属性
            feature.setAttributes(attributes);

            // 添加要素到图层
            provider->addFeature(feature);

            // 提交更改以更新内存图层
            newLayer->commitChanges();

            // 将新图层添加到项目
            QgsProject::instance()->addMapLayer(newLayer);

            // 刷新图层显示
            newLayer->triggerRepaint();
        }
        else { QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("矢量图层无效！")); }
    }
    else if (m_currentMode == Mode::Copy) {
        // 批量复制要素
        selectedFeatures = m_layer->selectedFeatures(); // 获取已选择的要素
        if (selectedFeatures.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("没有选中的要素"));
            return;
        }

        // 计算所有选中要素的质心中点
        QPointF totalCentroid(0.0, 0.0);

        for (const QgsFeature& feature : selectedFeatures) {
            QgsGeometry originalGeometry = feature.geometry();
            if (originalGeometry.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("选中的要素几何体为空"));
                continue;
            }
            QgsPointXY centroid = originalGeometry.centroid().asPoint();
            totalCentroid += QPointF(centroid.x(), centroid.y());
        }

        // 计算质心的中点
        double numFeatures = selectedFeatures.size();
        double averageX = totalCentroid.x() / numFeatures;
        double averageY = totalCentroid.y() / numFeatures;
        QgsPointXY centroidMidpoint(averageX, averageY); // 质心中点

        // 计算目标偏移量
        double offsetX = point.x() - centroidMidpoint.x(); // 相对偏移量
        double offsetY = point.y() - centroidMidpoint.y(); // 相对偏移量

        // 使用 QList 存储被复制的要素
        QList<QgsFeature> newFeatures;

        for (const QgsFeature& feature : selectedFeatures) {
            QgsFeature newFeature = feature; // 创建新要素的副本
            newFeature.setId(0); // 将ID设置为0，以便在将其添加到图层中时分配新的ID

            QgsGeometry originalGeometry = feature.geometry();

            // 检查要素的几何体是否有效
            if (originalGeometry.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("选中的要素几何体为空"));
                continue;
            }

            // 创建几何体副本进行平移
            QgsGeometry translatedGeometry = originalGeometry;
            translatedGeometry.translate(offsetX, offsetY); // 平移

            // 检查平移后的几何体是否有效
            if (translatedGeometry.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("平移后的几何体无效"));
                continue;
            }

            newFeature.setGeometry(translatedGeometry); // 更新几何到新位置

            // 获取图层的数据提供器并添加复制的要素
            QgsVectorDataProvider* provider = m_layer->dataProvider();
            if (!provider->addFeature(newFeature)) {
                QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("添加复制要素失败"));
            }

            newFeatures.append(newFeature);
        }
        // 记录撤销操作
        undoStack.append(UndoAction(COPY, newFeatures)); // 历史记录为复制要素

        m_layer->triggerRepaint(); // 刷新图层显示
        m_mapCanvas->refresh(); // 刷新地图画布
    }
    else if (m_currentMode == Mode::Move) {
        // 批量移动已选择的要素
        selectedFeatures = m_layer->selectedFeatures(); // 获取已选择的要素
        if (selectedFeatures.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("没有选中的要素"));
            return;
        }

        // 进入编辑模式
        if (!m_layer->isEditable() && !m_layer->startEditing()) {
            return; // 如果无法进入编辑模式，则返回
        }

        QList<QgsFeature> movedFeatures; // 用于存储被移动的特征
        QList<QgsGeometry> originalGeometries; // 用于记录原始几何体

        // 计算所有选中要素的质心中点
        QPointF totalCentroid(0.0, 0.0);
        for (const QgsFeature& feature : selectedFeatures) {
            QgsGeometry originalGeometry = feature.geometry();
            if (originalGeometry.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("选中的要素几何体为空"));
                continue;
            }
            QgsPointXY centroid = originalGeometry.centroid().asPoint();
            totalCentroid += QPointF(centroid.x(), centroid.y());
            originalGeometries.append(originalGeometry); // 记录原始几何体
        }

        // 计算质心的中点
        double numFeatures = selectedFeatures.size();
        double averageX = totalCentroid.x() / numFeatures;
        double averageY = totalCentroid.y() / numFeatures;
        QgsPointXY centroidMidpoint(averageX, averageY); // 质心中点

        // 计算目标偏移量
        double offsetX = point.x() - centroidMidpoint.x(); // 相对偏移量
        double offsetY = point.y() - centroidMidpoint.y(); // 相对偏移量

        // 移动所有要素
        for (QgsFeature& feature : selectedFeatures) {
            QgsGeometry originalGeometry = feature.geometry();
            if (originalGeometry.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("选中的要素几何体为空"));
                continue;
            }

            // 创建几何体副本进行平移
            QgsGeometry translatedGeometry = originalGeometry;
            translatedGeometry.translate(offsetX, offsetY); // 平移

            if (translatedGeometry.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("平移后的几何体无效"));
                continue;
            }

            // 更新要素的几何体
            feature.setGeometry(translatedGeometry);

            if (!m_layer->updateFeature(feature)) {
                QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("特征更新失败！"));
            }

            movedFeatures.append(feature); // 记录移动的特征
        }

        // 记录撤销操作，保存移动的特征及其原始几何
        undoStack.append(UndoAction(MOVE, movedFeatures, originalGeometries));

        if (!m_layer->commitChanges()) {
            QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("提交更改失败！"));
        }

        m_layer->triggerRepaint();
        m_mapCanvas->refresh();
    }
    else if (m_currentMode == Mode::View) {
    // 批量移动已选择的要素
    selectedFeatures = m_layer->selectedFeatures(); // 获取已选择的要素
    if (selectedFeatures.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("没有选中的要素"));
        return;
    }

    // 进入编辑模式
    if (!m_layer->isEditable() && !m_layer->startEditing()) {
        return; // 如果无法进入编辑模式，则返回
    }

    for (const QgsFeature& feature : selectedFeatures) {

        // 显示属性编辑对话框
        QDialog dialog(this);
        dialog.setWindowTitle(QStringLiteral("编辑属性"));

        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        QTableWidget* attributeTable = new QTableWidget(&dialog);

        // 设置表格行列数
        const QgsAttributes& attributes = feature.attributes();
        const QgsFields& fields = m_layer->fields();
        attributeTable->setRowCount(fields.size());
        attributeTable->setColumnCount(2);
        attributeTable->setHorizontalHeaderLabels({ QStringLiteral("字段名称"), QStringLiteral("字段值") });

        // 填充表格数据
        for (int i = 0; i < fields.size(); ++i) {
            const QgsField& field = fields[i];
            attributeTable->setItem(i, 0, new QTableWidgetItem(field.name()));
            attributeTable->setItem(i, 1, new QTableWidgetItem(attributes[i].toString()));
        }

        layout->addWidget(attributeTable);

        // 添加保存和取消按钮
        QPushButton* saveButton = new QPushButton(QStringLiteral("保存"), &dialog);
        style.setConfirmButton(saveButton);
        QPushButton* cancelButton = new QPushButton(QStringLiteral("取消"), &dialog);
        style.setConfirmButton(cancelButton);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(saveButton);
        buttonLayout->addWidget(cancelButton);
        layout->addLayout(buttonLayout);

        // 连接保存按钮
        connect(saveButton, &QPushButton::clicked, [&]() {
            QgsFeature updatedFeature = feature; // 创建副本以更新属性
            QgsAttributeMap updatedAttributes;   // 存储更新后的属性键值对

            for (int i = 0; i < fields.size(); ++i) {
                QString value = attributeTable->item(i, 1)->text();
                updatedAttributes.insert(i, value); // 插入更新的字段值
            }

            QMap<QgsFeatureId, QgsAttributeMap> attributeChanges;
            attributeChanges.insert(updatedFeature.id(), updatedAttributes);

            // 更新图层的属性值
            if (!m_layer->dataProvider()->changeAttributeValues(attributeChanges)) {
                QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("更新要素属性失败"));
            }
            else {
                QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("要素属性已更新"));
                m_mapCanvas->refresh(); // 刷新地图以显示更新
            }
            dialog.accept();
            });

        // 连接取消按钮
        connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

        // 显示对话框
        dialog.exec();
    }
}
}

// 选择要素
void MainInterface::on_selectFeatureAct_triggered()
{
    if (!m_isSelecting) m_isSelecting = true;
    // 查找矢量图层
    m_layer = nullptr;
    for (QgsMapLayer* mapLayer : m_mapCanvas->layers()) {
        if (mapLayer->type() == Qgis::LayerType::Vector) {
            m_layer = qobject_cast<QgsVectorLayer*>(mapLayer);
            break;
        }
    }

    if (!m_layer) {
        return;
    }

    if (!identifyTool) {
        identifyTool = new QgsMapToolIdentifyFeature(m_mapCanvas, m_layer);
        connect(identifyTool, SIGNAL(featureIdentified(const QgsFeature&)), this, SLOT(onFeatureIdentified(const QgsFeature&)));
        m_mapCanvas->setMapTool(identifyTool);
    }
}

// 选中要素操作
void MainInterface::onFeatureIdentified(const QgsFeature& feature) {
    // 切换选择状态
    if (selectedFeatureIds.contains(feature.id())) {
        // 如果已经选中，取消选择
        selectedFeatureIds.remove(feature.id());
        m_layer->removeSelection();
    }
    else {
        // 如果未选中，则添加选择
        selectedFeatureIds.insert(feature.id());
        m_layer->select(feature.id());
    }
}

// 复制要素
void MainInterface::on_copyFeatureAct_triggered()
{
    if (m_isSelecting == 1) {
        m_currentMode = Mode::Copy;
        // 创建 QGIS 绘制工具用于点要素输入
        QgsMapToolEmitPoint* tool = new QgsMapToolEmitPoint(m_mapCanvas);
        connect(tool, &QgsMapToolEmitPoint::canvasClicked, this, &MainInterface::onCanvasReleased);
        m_mapCanvas->setMapTool(tool);
    }else QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("请先进入选择模式！"));
}

// 移动要素
void MainInterface::on_moveFeatureAct_triggered(){
    if (m_isSelecting == 1) {
       m_currentMode = Mode::Move;

       // 创建 QGIS 绘制工具用于点要素输入
       QgsMapToolEmitPoint* tool = new QgsMapToolEmitPoint(m_mapCanvas);
       connect(tool, &QgsMapToolEmitPoint::canvasClicked, this, &MainInterface::onCanvasReleased);
       m_mapCanvas->setMapTool(tool);
    }
    else QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("请先进入选择模式！"));
}

// 删除要素
void MainInterface::on_deleteFeatureAct_triggered()
{
    if (m_isSelecting == 1) {
        // 使用 QList 存储被删除的要素
        QList<QgsFeature> deleteFeatures;

        // 检查是否有选中的要素ID
        if (selectedFeatureIds.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("没有选中的特征"));
            return;
        }

        // 根据选中的要素ID获取要删除的要素
        for (qint64 featureId : selectedFeatureIds) {
            QgsFeature feature = m_layer->getFeature(featureId);
            if (feature.isValid()) {
                deleteFeatures.append(feature); // 添加要删除的要素
            }
        }

        // 记录撤销操作
        undoStack.append(UndoAction(DELETE, deleteFeatures)); // 记录删除操作

        // 删除当前选中的要素
        for (const QgsFeature& feature : deleteFeatures) {
            m_layer->deleteFeature(feature.id());
        }

        // 清空已选择的要素ID集合
        selectedFeatureIds.clear();

        // 刷新地图
        m_mapCanvas->refresh();
        qDebug() << "Deleted Features" << deleteFeatures.size();
    }
    else QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("请先进入选择模式！"));
}

// 输入注记
void MainInterface::textLabel(const QgsPointXY& point, Qt::MouseButton button) {

    if (!isEditing) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("编辑模式未启用，无法添加注记"));
        return;
    }

    // 弹出对话框获取注记内容
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("添加文字注记"));
    dialog.resize(550, 200);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QLabel* label = new QLabel(QStringLiteral("text"), &dialog);
    QLineEdit* textInput = new QLineEdit(&dialog);
    style.setLineEdit(textInput);
    textInput->setPlaceholderText(QStringLiteral("输入示例：a<sup>1</sup><sub>2</sub>"));
    layout->addWidget(label);
    layout->addWidget(textInput);

    QPushButton* okButton = new QPushButton(QStringLiteral("确定"), &dialog);
    QPushButton* cancelButton = new QPushButton(QStringLiteral("取消"), &dialog);
    style.setConfirmButton(okButton);
    style.setConfirmButton(cancelButton);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return; // 用户取消操作
    }

    QString annotationText = textInput->text();
    if (annotationText.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("注记内容不能为空"));
        return;
    }

    // 创建注记内容，允许使用HTML格式支持上下标
    QTextDocument* doc = new QTextDocument();

    // 设置注记内容为 HTML 格式，支持上下标
    doc->setHtml(annotationText);  // 直接使用 HTML 格式来支持 <sup> 和 <sub>

    // 可以根据需求设置注记的样式
    QString style = "body { margin: 5px; font-size: 12px; }";
    doc->setDefaultStyleSheet(style);

    // 创建文字注记并添加到地图
    QgsTextAnnotation* annotation = new QgsTextAnnotation();
    annotation->setMapPosition(point); // 设置注记的地理位置
    annotation->setDocument(doc); // 设置注记内容为富文本

    // 使用 QgsMapCanvasAnnotationItem 包装注记
    QgsMapCanvasAnnotationItem* annotationItem = new QgsMapCanvasAnnotationItem(annotation, m_mapCanvas);

    // 添加注记项到容器
    m_annotations.push_back(annotationItem);

    undoStack.append(UndoAction(LABEL)); // 历史记录为注记

    annotationItem->setVisible(true); // 显示注记
}

// 合并选中的要素
void MainInterface::mergeFeatures(const QList<QgsFeature>& features)
{
    if (features.size() < 2) {
        QMessageBox::information(this, QStringLiteral("要素数量不够"), QStringLiteral("请选择至少两个要素去合并."));
        return;
    }

    // 查找矢量图层
    m_layer = nullptr;
    for (QgsMapLayer* mapLayer : m_mapCanvas->layers()) {
        if (mapLayer->type() == Qgis::LayerType::Vector) {
            m_layer = qobject_cast<QgsVectorLayer*>(mapLayer);
            break;
        }
    }

    // 获取当前矢量图层
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(m_layer);
    if (!vectorLayer) {
        QMessageBox::warning(this, QStringLiteral("图层无效"), QStringLiteral("当前图层不是矢量图层."));
        return;
    }


    // 合并选中的几何
    QgsGeometry mergedGeometry = features.first().geometry();
    for (int i = 1; i < features.size(); ++i) {
        mergedGeometry = mergedGeometry.combine(features[i].geometry());
    }


    // 创建合并后的新要素
    QgsFeature mergedFeature;
    mergedFeature.setGeometry(mergedGeometry);


    // 开始图层编辑
    if (!vectorLayer->isEditable()) {
        vectorLayer->startEditing();
    }

    // 删除原有要素并添加合并后的要素
    // 确保合并后的要素添加到当前图层
    QgsVectorLayerEditBuffer* editBuffer = vectorLayer->editBuffer();

    for (const QgsFeature& feature : features) {
        editBuffer->deleteFeature(feature.id());
    }

    // 添加合并后的新要素
    vectorLayer->dataProvider()->addFeature(mergedFeature);  // 添加到编辑缓冲区

    onFeatureIdentified(mergedFeature);

    // 提交修改
    vectorLayer->commitChanges();  // 提交所有更改

    // 刷新图层的渲染（触发图层的重绘）
    vectorLayer->triggerRepaint();

    // 强制刷新地图画布，确保合并后的要素显示出来
    m_mapCanvas->refresh();

    // 提交修改并刷新图层
    vectorLayer->commitChanges();
    vectorLayer->triggerRepaint();
    m_mapCanvas->refresh();


    QMessageBox::information(this, QStringLiteral("合并成功！"), QStringLiteral("选择的要素已经合并！"));
}

// 合并面
void MainInterface::on_faceMergeAct_triggered() {
    
    if (m_isSelecting == 1) {
    QList<QgsFeature> mergerFeatures;

    // 检查是否有选中的要素ID
    if (selectedFeatureIds.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("没有选中的特征"));
        return;
    }

    // 根据选中的要素ID获取要删除的要素
    for (qint64 featureId : selectedFeatureIds) {
        QgsFeature feature = m_layer->getFeature(featureId);
        if (feature.isValid()) {
            mergerFeatures.append(feature); // 添加要删除的要素
        }
    }
    mergeFeatures(mergerFeatures);
    }
    else QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("请先进入选择模式！"));
}

// 双击图层函数
void MainInterface::onLayerTreeItemDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) {
        qDebug() << "Invalid index passed to the handler";
        return;
    }

    // 获取图层树的代理模型
    QgsLayerTreeProxyModel* proxyModel = qobject_cast<QgsLayerTreeProxyModel*>(m_layerTreeView->model());
    if (!proxyModel) {
        qDebug() << "Failed to cast to QgsLayerTreeProxyModel";
        return;
    }

    // 获取源模型
    QgsLayerTreeModel* model = qobject_cast<QgsLayerTreeModel*>(proxyModel->sourceModel());
    if (!model) {
        qDebug() << "Failed to get source QgsLayerTreeModel from proxy model";
        return;
    }

    // 使用代理模型的索引映射到源模型的索引
    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    if (!sourceIndex.isValid()) {
        qDebug() << "Invalid source index";
        return;
    }

    // 获取图层节点
    QgsLayerTreeNode* node = model->index2node(sourceIndex);
    if (!node) {
        qDebug() << "Node not found for the given source index";
        return;
    }

    if (QgsLayerTree::isLayer(node))
    {
        QgsMapLayer* layer = static_cast<QgsLayerTreeLayer*>(node)->layer();
        if (layer)
        {
            if (QgsVectorLayer* vectorLayer = dynamic_cast<QgsVectorLayer*>(layer))
            {
                // 打开矢量图层样式编辑器
                LayerStyle* editor = new LayerStyle(this);
                if (editor->exec() == QDialog::Accepted)
                {
                    // 获取样式设置并应用到图层
                    QColor color = editor->getColor();
                    int size = editor->getSize();
                    double opacity = editor->getOpacity();

                    // 更新图层样式（矢量图层）
                    QgsSymbol* symbol = QgsSymbol::defaultSymbol(vectorLayer->geometryType());
                    symbol->setColor(color);
                    symbol->setOpacity(opacity);
                    if (QgsMarkerSymbol* markerSymbol = dynamic_cast<QgsMarkerSymbol*>(symbol))
                    {
                        markerSymbol->setSize(size);
                    }
                    vectorLayer->setRenderer(new QgsSingleSymbolRenderer(symbol));
                    vectorLayer->triggerRepaint();
                }
                delete editor;
            }
            else if (QgsRasterLayer* rasterLayer = dynamic_cast<QgsRasterLayer*>(layer))
            {
                // 打开栅格图层样式编辑器
                RasterLayerStyleEditor* rasterEditor = new RasterLayerStyleEditor(rasterLayer, this);
                if (rasterEditor->exec() == QDialog::Accepted)
                {
                    // 获取透明度和色表
                    double opacity = rasterEditor->getOpacity();
                    QString colorRamp = rasterEditor->getColorRamp();

                    rasterLayer->triggerRepaint();  // 刷新图层显示
                }
                delete rasterEditor;
            }
            else
            {
                qDebug() << "The selected layer is neither a vector nor a raster layer.";
            }
        }
        else
        {
            qDebug() << "Failed to get the layer from the node.";
        }
    }
}

// 符号库按钮
void MainInterface::on_symbolAct_triggered()
{
    symbolLibrary->setCurrentLayer(m_layer);  // 设置当前图层
    symbolLibrary->exec();
}

// 图层改变函数
void MainInterface::onCurrentLayerChanged(QgsMapLayer* layer)
{
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);
    if (vectorLayer)
    {
        //判断是否为你想要处理的图层类型，例如shapefile
        if (vectorLayer->dataProvider()->name() == "ogr" && vectorLayer->source().endsWith(".shp"))
        {
            //获取矢量图层的路径
            m_curMapLayer = vectorLayer;
        }
    }
    else if (QgsRasterLayer* rasterLayer = qobject_cast<QgsRasterLayer*>(layer))
    {
        m_curMapLayer = rasterLayer;
    }
}

// 分割面
void MainInterface::on_divideSurfaceAct_triggered() {
    PlaneOfDivision* divide = new PlaneOfDivision(m_mapCanvas, this);
    divide->show();
}

// 边界提取
void MainInterface::on_extractBoundaryAct_triggered(){
    if (!m_curMapLayer || (m_curMapLayer->type() != Qgis::LayerType::Vector)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请选择有效的矢量图层"));
        return;
    }

    // 1. 获取输入图层
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(m_curMapLayer);
    if (!vectorLayer) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法获取有效的矢量图层"));
        return;
    }

    // 2. 允许用户选择输出文件路径或临时图层
    QString defaultOutputFileName = QStringLiteral("D:/code/GISdesignanddevelopment/Data/Boundary_Layer/") + m_curMapLayer->name() + "_Boundary.shp";
    QString outputFileName = QFileDialog::getSaveFileName(this, QStringLiteral("选择输出文件"),
        defaultOutputFileName,
        QStringLiteral("Shapefile (*.shp);;GeoPackage (*.gpkg);;其他格式"));
    if (outputFileName.isEmpty()) {
        return; // 用户取消选择输出文件
    }

    // 3. 设置boundary算法的参数
    QVariantMap parameters;
    parameters.insert("INPUT", QVariant::fromValue(vectorLayer)); // 输入图层
    parameters.insert("OUTPUT", outputFileName);                  // 输出路径

    // 4. 获取并执行boundary算法
    QgsProcessingAlgorithm* alg = QgsApplication::processingRegistry()->createAlgorithmById("native:boundary");
    if (!alg) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法获取边界提取算法"));
        return;
    }

    // 设置处理上下文和反馈
    QgsProcessingFeedback feedback;
    QgsProcessingContext context;

    // 执行算法
    bool success = false;
    QVariantMap result = alg->run(parameters, context, &feedback, &success);

    if (success) {
        QMessageBox::information(this, QStringLiteral("提取完成"), QStringLiteral("边界已成功提取"));

        // 将提取后的边界图层加载到项目中
        QString outputFilePath = result["OUTPUT"].toString();
        if (outputFilePath.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("输出路径为空"));
            return;
        }

        QString boundaryLayerName = vectorLayer->name() + "_Boundary";

        QgsVectorLayer* boundaryLayer = new QgsVectorLayer(outputFilePath, boundaryLayerName, "ogr");
        if (boundaryLayer->isValid()) {
            QgsProject::instance()->addMapLayer(boundaryLayer);
            QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("边界图层已成功添加到项目中"));
        }
        else {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法加载边界图层"));
        }
    }
    else {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("提取边界算法执行失败"));
    }
}

// 延长线
void MainInterface::on_extentLineAct_triggered()
{
    if (!m_curMapLayer || m_curMapLayer->type() != Qgis::LayerType::Vector) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请选择有效的矢量图层"));
        return;
    }

    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(m_curMapLayer);

    // 获取图层名称并生成默认输出文件名
    QString layerName = vectorLayer->name();
    QString defaultOutputFileName = QStringLiteral("D:/code/GISdesignanddevelopment/Data/Extended_Lines/") + layerName + QStringLiteral("_ExtendedLines.shp");

    // 弹出文件选择对话框让用户选择输出路径
    QString outputFileName = QFileDialog::getSaveFileName(this,
        QStringLiteral("选择输出文件"),
        defaultOutputFileName,
        QStringLiteral("Shapefile (*.shp)"));

    if (outputFileName.isEmpty()) {
        // 用户取消选择，退出操作
        return;
    }

    // 弹出输入框让用户输入起始点和结束点的延长距离（以逗号分隔）
    QDialog dialog(this);
    dialog.resize(600, 200);
    dialog.setWindowTitle(QStringLiteral("设置延长距离"));

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // 创建 QLineEdit
    QLineEdit* lineEdit = new QLineEdit(&dialog);
    lineEdit->setPlaceholderText(QStringLiteral("0.01, 0.01")); // 提示文本
    style.setLineEdit(lineEdit);
    layout->addWidget(lineEdit);

    // 创建按钮
    QPushButton* okButton = new QPushButton(QStringLiteral("确定"), &dialog);
    QPushButton* cancelButton = new QPushButton(QStringLiteral("取消"), &dialog);

    // 美化按钮
    style.setConfirmButton(okButton);
    style.setConfirmButton(cancelButton);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    // 连接按钮的信号
    bool accepted = false;
    connect(okButton, &QPushButton::clicked, [&]() {
        accepted = true;
        dialog.close();
        });
    connect(cancelButton, &QPushButton::clicked, [&]() {
        dialog.close();
        });

    dialog.exec(); // 显示对话框并等待用户输入

    // 获取用户输入
    QString input = lineEdit->text();
    if (!accepted || input.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("输入延长距离失败"));
        return; // 处理输入失败
    }

    // 分割用户输入的字符串为起始点和结束点的距离
    QStringList distances = input.split(',');
    if (distances.size() != 2) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请输入有效的格式，例如：0.01, 0.01"));
        return;
    }

    bool okStart, okEnd;
    double startDistance = distances.at(0).toDouble(&okStart);
    double endDistance = distances.at(1).toDouble(&okEnd);

    if (!okStart || !okEnd) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无效的距离输入"));
        return;
    }

    // 设置延长线算法的参数
    QVariantMap parameters;
    parameters.insert("INPUT", QVariant::fromValue(vectorLayer));              // 设置输入图层
    parameters.insert("START_DISTANCE", startDistance);                       // 设置起始点延长距离
    parameters.insert("END_DISTANCE", endDistance);                           // 设置结束点延长距离
    parameters.insert("OUTPUT", outputFileName);                              // 设置输出路径

    // 获取延长线算法
    QgsProcessingAlgorithm* alg = QgsApplication::processingRegistry()->createAlgorithmById("native:extendlines");

    if (alg) {
        // 设置反馈信息和上下文
        QgsProcessingFeedback feedback;
        QgsProcessingContext context;

        // 执行算法
        bool ok = false;
        QVariantMap result = alg->run(parameters, context, &feedback, &ok);

        if (ok) {
            QMessageBox::information(this, QStringLiteral("延长完成"), QStringLiteral("线图层延长操作完成"));

            // 获取输出文件路径
            QString outputFilePath = result["OUTPUT"].toString();
            if (outputFilePath.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("输出路径为空"));
                return;
            }

            // 使用原图层名称 + "_ExtendedLines" 作为新的图层名称
            QString extendedLayerName = layerName + "_ExtendedLines";

            // 创建新的矢量图层
            QgsVectorLayer* extendedLayer = new QgsVectorLayer(outputFilePath, extendedLayerName, "ogr");
            if (extendedLayer->isValid()) {
                // 将新的矢量图层添加到项目中
                QgsProject::instance()->addMapLayer(extendedLayer);
                QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("延长后的线图层已成功添加到项目中"));
            }
            else {
                QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法加载延长后的线图层"));
            }
        }
        else {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("延长线算法执行失败"));
        }
    }
    else {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法获取延长线算法"));
    }
}

// 旋转线
void MainInterface::on_rotateLineAct_triggered()
{
    if (!m_curMapLayer || m_curMapLayer->type() != Qgis::LayerType::Vector) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请选择有效的矢量图层"));
        return;
    }

    // 弹出输入框让用户输入旋转角度
    QDialog dialog(this);
    dialog.resize(600, 200);
    dialog.setWindowTitle(QStringLiteral("设置旋转角度"));

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // 创建提示文本和 QLineEdit
    QLabel* label = new QLabel(QStringLiteral("请输入旋转角度（-360 到 360）："), &dialog);
    layout->addWidget(label);

    QLineEdit* lineEdit = new QLineEdit(&dialog);
    style.setLineEdit(lineEdit);
    lineEdit->setPlaceholderText(QStringLiteral("0.0")); // 提示文本
    layout->addWidget(lineEdit);

    // 创建按钮
    QPushButton* okButton = new QPushButton(QStringLiteral("确定"), &dialog);
    style.setConfirmButton(okButton);
    QPushButton* cancelButton = new QPushButton(QStringLiteral("取消"), &dialog);
    style.setConfirmButton(cancelButton);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    // 连接按钮的信号
    connect(okButton, &QPushButton::clicked, [&]() {
        bool ok;
        double angle = lineEdit->text().toDouble(&ok); // 获取用户输入并转换为 double
        if (ok && angle >= -360.0 && angle <= 360.0) {
            // 允许用户选择输出文件路径
            QString defaultOutputFileName = QStringLiteral("D:/code/GISdesignanddevelopment/Data/Rotated_Lines/") + m_curMapLayer->name() + "_Rotated.shp";
            QString outputFileName = QFileDialog::getSaveFileName(this, QStringLiteral("选择输出文件"),
                defaultOutputFileName,
                QStringLiteral("Shapefile (*.shp)"));
            if (outputFileName.isEmpty()) {
                return; // 用户取消选择输出文件
            }

            // 设置旋转算法的参数
            QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(m_curMapLayer);
            QVariantMap parameters;
            parameters.insert("INPUT", QVariant::fromValue(vectorLayer)); // 输入图层
            parameters.insert("ANGLE", angle); // 旋转角度
            parameters.insert("CENTER", ""); // 旋转中心（默认）
            parameters.insert("OUTPUT", outputFileName); // 输出文件路径

            // 获取旋转算法
            QgsProcessingAlgorithm* alg = QgsApplication::processingRegistry()->createAlgorithmById("native:rotatefeatures");
            if (!alg) {
                QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法获取旋转算法"));
                return;
            }

            // 设置处理上下文和反馈
            QgsProcessingFeedback feedback;
            QgsProcessingContext context;

            // 执行旋转算法
            bool success = false;
            QVariantMap result = alg->run(parameters, context, &feedback, &success);

            if (success) {
                QMessageBox::information(this, QStringLiteral("旋转完成"), QStringLiteral("矢量图层已成功旋转"));

                // 将旋转后的图层加载到项目中
                QString outputFilePath = result["OUTPUT"].toString();
                if (outputFilePath.isEmpty()) {
                    QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("输出路径为空"));
                    return;
                }

                QString rotatedLayerName = vectorLayer->name() + "_Rotated";

                QgsVectorLayer* rotatedLayer = new QgsVectorLayer(outputFilePath, rotatedLayerName, "ogr");
                if (rotatedLayer->isValid()) {
                    QgsProject::instance()->addMapLayer(rotatedLayer);
                    QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("旋转后的图层已成功添加到项目中"));
                }
                else {
                    QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法加载旋转后的图层"));
                }
            }
            else {
                QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("旋转算法执行失败"));
            }
            dialog.close(); // 关闭对话框
        }
        else {
            // 提示用户输入无效
            QMessageBox::warning(this, QStringLiteral("输入错误"), QStringLiteral("请输入有效的角度范围（-360 到 360）。"));
        }
        });

    connect(cancelButton, &QPushButton::clicked, [&]() {
        dialog.close(); // 关闭对话框
        });

    dialog.exec(); // 显示对话框并等待用户输入
};

// 光滑线
void MainInterface::on_smoothGeometryAct_triggered()
{
    if (!m_curMapLayer || m_curMapLayer->type() != Qgis::LayerType::Vector) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请选择有效的矢量图层"));
        return;
    }

    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(m_curMapLayer);

    // 获取图层名称并生成默认输出文件名
    QString layerName = vectorLayer->name();
    QString defaultOutputFileName = QStringLiteral("D:/code/GISdesignanddevelopment/Data/Smoothed_results/") + layerName + QStringLiteral("_Smoothed.shp");

    // 弹出文件选择对话框让用户选择输出路径
    QString outputFileName = QFileDialog::getSaveFileName(this,
        QStringLiteral("选择输出文件"),
        defaultOutputFileName,
        QStringLiteral("Shapefile (*.shp)"));

    if (outputFileName.isEmpty()) {
        // 用户取消选择，退出操作
        return;
    }

    // 设置光滑算法的参数
    QVariantMap parameters;
    parameters.insert("INPUT", QVariant::fromValue(vectorLayer));  // 设置输入图层
    parameters.insert("ITERATIONS", 4);                            // 设置迭代次数
    parameters.insert("OFFSET", 0.25);                              // 设置偏移量
    parameters.insert("MAX_ANGLE", 180.0);                         // 设置最大角度
    parameters.insert("OUTPUT", outputFileName);                   // 设置输出路径

    // 获取光滑几何算法
    QgsProcessingAlgorithm* alg = QgsApplication::processingRegistry()->createAlgorithmById("native:smoothgeometry");

    if (alg) {
        // 设置反馈信息和上下文
        QgsProcessingFeedback feedback;
        QgsProcessingContext context;

        // 执行算法
        bool ok = false;
        QVariantMap result = alg->run(parameters, context, &feedback, &ok);

        if (ok) {
            QMessageBox::information(this, QStringLiteral("光滑完成"), QStringLiteral("矢量图层几何光滑完成"));

            // 获取输出文件路径
            QString outputFilePath = result["OUTPUT"].toString();
            if (outputFilePath.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("输出路径为空"));
                return;
            }

            // 使用原图层名称 + "_Smoothed" 作为新的图层名称
            QString smoothedLayerName = layerName + "_Smoothed";

            // 创建新的矢量图层
            QgsVectorLayer* smoothedLayer = new QgsVectorLayer(outputFilePath, smoothedLayerName, "ogr");
            if (smoothedLayer->isValid()) {
                // 将新的矢量图层添加到项目中
                QgsProject::instance()->addMapLayer(smoothedLayer);
                QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("光滑后的图层已成功添加到项目中"));
            }
            else {
                QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法加载光滑后的图层"));
            }
        }
        else {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("几何光滑算法执行失败"));
        }
    }
    else {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法获取几何光滑算法"));
    }
}

// 简化线
void MainInterface::on_simplifyLineAct_triggered() {
    if (!m_curMapLayer || (m_curMapLayer->type() != Qgis::LayerType::Vector)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请选择有效的矢量图层"));
        return;
    }

    //  提示用户选择简化方法
    QStringList methodOptions = {
        QStringLiteral("0-Distance (Douglas-Peucker)"),
        QStringLiteral("1-Snap to grid"),
        QStringLiteral("2-Area (Visvalingam)")
    };

    bool ok;
    QString method = QInputDialog::getItem(this, QStringLiteral("选择简化方法"),
        QStringLiteral("选择简化方法："),
        methodOptions, 0, false, &ok);
    if (!ok) {
        return; // 用户取消选择
    }

    // 设置容差值
    double tolerance = QInputDialog::getDouble(this, QStringLiteral("设置容差"),
        QStringLiteral("请输入简化容差值："),
        1.0, 0.0, 10000.0, 1, &ok);
    if (!ok) {
        return; // 用户取消或输入无效的值
    }

    // 允许用户选择输出文件路径或临时图层
    QString defaultOutputFileName = QStringLiteral("D:/code/GISdesignanddevelopment/Data/Simplified_Geometries/") + m_curMapLayer->name() + "_Simplified.shp";
    QString outputFileName = QFileDialog::getSaveFileName(this, QStringLiteral("选择输出文件"),
        defaultOutputFileName,
        QStringLiteral("Shapefile (*.shp);;GeoPackage (*.gpkg);;其他格式"));
    if (outputFileName.isEmpty()) {
        return; // 用户取消选择输出文件
    }

    // 设置简化算法的参数
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(m_curMapLayer);
    QVariantMap parameters;
    parameters.insert("INPUT", QVariant::fromValue(vectorLayer)); // 输入图层
    parameters.insert("METHOD", methodOptions.indexOf(method));   // 简化方法（0, 1, 2）
    parameters.insert("TOLERANCE", tolerance);                    // 容差值
    parameters.insert("OUTPUT", outputFileName);                  // 输出路径

    // 获取并执行简化算法
    QgsProcessingAlgorithm* alg = QgsApplication::processingRegistry()->createAlgorithmById("native:simplifygeometries");
    if (!alg) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法获取简化算法"));
        return;
    }

    // 设置处理上下文和反馈
    QgsProcessingFeedback feedback;
    QgsProcessingContext context;

    // 执行简化算法
    bool success = false;
    QVariantMap result = alg->run(parameters, context, &feedback, &success);

    if (success) {
        QMessageBox::information(this, QStringLiteral("简化完成"), QStringLiteral("几何图形已成功简化"));

        // 将简化后的图层加载到项目中
        QString outputFilePath = result["OUTPUT"].toString();
        if (outputFilePath.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("输出路径为空"));
            return;
        }

        QString simplifiedLayerName = vectorLayer->name() + "_Simplified";

        QgsVectorLayer* simplifiedLayer = new QgsVectorLayer(outputFilePath, simplifiedLayerName, "ogr");
        if (simplifiedLayer->isValid()) {
            QgsProject::instance()->addMapLayer(simplifiedLayer);
            QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("简化后的图层已成功添加到项目中"));
        }
        else {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法加载简化后的图层"));
        }
    }
    else {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("简化算法执行失败"));
    }
}

// 复位
void MainInterface::on_resetAct_triggered()
{
    // 如果当前选中的图层有效，且存在该图层的初始范围
    if (m_curMapLayer && m_layerExtents.contains(m_curMapLayer))
    {
        QgsRectangle extent = m_layerExtents[m_curMapLayer];
        if (!extent.isNull())
        {
            m_mapCanvas->setExtent(extent);  // 恢复当前图层的初始视图范围
            m_mapCanvas->refresh();  // 刷新地图画布
        }
        else
        {
            QMessageBox::information(0, "", "当前图层没有有效的视图范围");
        }
    }
    else
    {
        QMessageBox::information(0, "", "没有选中图层或该图层没有存储初始范围");
    }
}

// 撤销操作
void MainInterface::undo() {
    if (undoStack.isEmpty()) return;

    UndoAction lastAction = undoStack.takeLast(); // 获取最后一个操作

    switch (lastAction.type) {
    case ADD:
        // 从图层中删除所有要素
        for (const QgsFeature& feature : lastAction.features) {
            m_layer->deleteFeature(feature.id());
        }
        // 更新图层显示
        m_layer->triggerRepaint();
        break;

    case DELETE:
        // 从图层中加入所有要素
        for (const QgsFeature& feature : lastAction.features) {
            QgsFeature newFeature = feature;
            // 获取图层的数据提供者
            QgsVectorDataProvider* provider = m_layer->dataProvider();
            m_layer->dataProvider()->addFeature(newFeature);
        }
        // 更新图层显示
        m_layer->triggerRepaint();
        break;

    case MOVE:
        // 恢复到原始几何体
        for (size_t i = 0; i < lastAction.features.size(); ++i) {
            QgsFeature& feature = lastAction.features[i];
            QgsGeometry originalGeometry = lastAction.originalGeometries[i]; // 获取原始几何体

            // 更新特征的几何体
            feature.setGeometry(originalGeometry);

            if (!m_layer->updateFeature(feature)) {
                QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("特征更新失败！"));
            }
        }
        break;

    case COPY:
        // 从图层中删除所有复制的要素
        for (const QgsFeature& feature : lastAction.features) {
            m_layer->deleteFeature(feature.id());
        }
        // 更新图层显示
        m_layer->triggerRepaint();
        break;

    case LABEL:
        // 获取最后一个注记
        QgsMapCanvasAnnotationItem* lastItem = m_annotations.back();
        // 从容器中移除
        m_annotations.pop_back();
        // 从画布中删除该注记
        m_mapCanvas->scene()->removeItem(lastItem);
        break;
    }

    redoStack.append(lastAction);
}

// 重做操作
void MainInterface::redo() {
    if (redoStack.isEmpty()) return;

    UndoAction lastAction = redoStack.takeLast(); // 获取最后一个重做操作

    switch (lastAction.type) {
    case ADD:
        // 将所有要素添加回图层
        for (const QgsFeature& feature : lastAction.features) {
            QgsFeature newFeature = feature;
            m_layer->dataProvider()->addFeature(newFeature);
        }
        // 更新图层显示
        m_layer->triggerRepaint();
        break;

    case DELETE:
        // 从图层中删除所有要素
        for (const QgsFeature& feature : lastAction.features) {
            m_layer->deleteFeature(feature.id());
        }
        // 更新图层显示
        m_layer->triggerRepaint();
        break;

    case MOVE:
        //// 恢复到被移动后的几何体
        //for (size_t i = 0; i < lastAction.features.size(); ++i) {
        //    QgsFeature& feature = lastAction.features[i];
        //    QgsGeometry newGeometry = lastAction.movedGeometries[i]; // 获取移动后几何体

        //    // 更新特征的几何体
        //    feature.setGeometry(newGeometry);

        //    if (!m_layer->updateFeature(feature)) {
        //        QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("特征更新失败！"));
        //    }
        //}
        break;

    case COPY:
        // 将所有要素添加回图层
        for (const QgsFeature& feature : lastAction.features) {
            QgsFeature newFeature = feature;
            m_layer->dataProvider()->addFeature(newFeature);
        }
        // 更新图层显示
        m_layer->triggerRepaint();
        break;

    case LABEL:
        //// 重新添加注记
        //QgsMapCanvasAnnotationItem* itemToRedo = new QgsMapCanvasAnnotationItem(/* 注记创建参数 */);
        //m_annotations.push_back(itemToRedo); // 将注记加入到当前注记列表中
        //m_mapCanvas->scene()->addItem(itemToRedo); // 将注记添加回画布
        break;
    }

    undoStack.append(lastAction); // 将已执行的操作移回到撤销栈中
}




