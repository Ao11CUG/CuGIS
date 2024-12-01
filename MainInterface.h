#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MainInterface.h"
#include "qgsmapcanvas.h"
#include <qgslayertree.h>
#include <qgslayertreeview.h>
#include "qgslayertreemodel.h"
#include <QgsMapLayer.h>
#include <qgslayertreemapcanvasbridge.h>
#include <qgslayertreeviewdefaultactions.h>
#include "EagleEye.h"
#include "ToolBox.h"
#include "Rasterize.h"
#include "BufferAnalysisFunction.h"
#include "VectorClippingFunction.h"
#include <qgsmaptooledit.h>
#include <qgsrubberband.h>
#include <qgsmapmouseevent.h>
#include <qgsmaptoolemitpoint.h>
#include <qgsfeature.h>
#include <qgspointXY.h>
#include <qgsfield.h>
#include<qgsproject.h>
#include<qgsmarkersymbol.h>
#include <qgsmapcanvasitem.h>
#include <qgsmaptoolextent.h>
#include <qgsmaptoolidentify.h>
#include <qgsmaptoolidentifyfeature.h>
#include <qgshighlight.h>
#include "Mode.h"
#include "UndoAction.h"
#include "StyleSheet.h"
#include "LayerStyle.h"
#include "RasterLayerStyleEditor.h"  
#include "SymbolLibManager.h"
#include <QDomDocument>
#include <qgsvectorlayereditbuffer.h>
#include "csvReader.h"
#include <qgstextannotation.h>
#include <qgsmapcanvasannotationitem.h>
#include "Attribute.h"
#include "RasterCalculator.h"
#include "PlaneOfDivision.h"

class MainInterface : public QMainWindow
{
    Q_OBJECT

public:
    MainInterface(QWidget* parent = nullptr);
    ~MainInterface();

    Mode m_currentMode; // 当前操作模式

private slots:
    void on_openVectorFileAct_triggered();
    void on_openRasterFileAct_triggered();
    void on_openProjectFileAct_triggered();
    void on_saveFileAct_triggered();

    void on_fullScreenAct_triggered();

    void on_openLayerTreeAct_triggered();
    void on_openEagleEyeAct_triggered();
    void on_openToolBoxAct_triggered();

    void on_rasterizeAct_triggered();
    void on_rasterCalculatorAct_triggered();
    void on_bufferAnalysisAct_triggered();
    void on_vectorTailorAct_triggered();
    void on_csvToVectorAct_triggered();
    void on_extentLineAct_triggered();      //延长线
    void on_rotateLineAct_triggered();  //旋转线
    void on_smoothGeometryAct_triggered();  //光滑线
    void on_divideSurfaceAct_triggered();
    void on_simplifyLineAct_triggered();
    void on_extractBoundaryAct_triggered();
    void removeLayer();
    void on_setCrsAct_triggered();

    void on_addPointAct_triggered();//添加点
    void on_selectFeatureAct_triggered();//选择要素
    void on_copyFeatureAct_triggered();
    void on_moveFeatureAct_triggered();
    void on_startEditingAct_triggered();//开始编辑
    void on_exitAct_triggered();
    void textLabel(const QgsPointXY& point, Qt::MouseButton button);

    void onLayerTreeItemDoubleClicked(const QModelIndex& index);//双击图层函数
    void on_viewAttributeAct_triggered();
    void on_symbolAct_triggered();
    void on_resetAct_triggered();

    //合并区相关函数
    void on_faceMergeAct_triggered();
    void onCurrentLayerChanged(QgsMapLayer* layer);

    //删除点
    void on_deleteFeatureAct_triggered();

    void onCanvasReleased(const QgsPointXY& point, Qt::MouseButton button);//添加点工具响应槽函数
    void onFeatureIdentified(const QgsFeature& feature);

    void onTreeViewContextMenu(const QPoint& pos);

    void undo();
    void redo();

private:
    Ui::MainInterfaceClass ui;

    void toggleLayerEditing(QgsVectorLayer* layer);

    void setStyle();
    StyleSheet style;

    Rasterize* rasterize;
    BufferAnalysisFunction* bufferAnalysis;
    VectorClippingFunction* vectorClip;
    csvReader* m_csvReader;
    Attribute* m_attribute;
    PlaneOfDivision* divide;

    std::vector<QgsMapCanvasAnnotationItem*> m_annotations; // 存储注记项

    // 地图画布
    QgsMapCanvas* m_mapCanvas;

    //鹰眼图
    EagleEye* m_eagleEyeWidget;
    void initEagleEye(QgsMapCanvas* mainCanvas);

    // 图层管理器
    void initLayerTreeView();
    QgsLayerTreeView* m_layerTreeView;
    QgsLayerTreeMapCanvasBridge* m_layerTreeCanvasBridge;

    // 工具箱
    ToolBox* m_toolbox;
    void initToolBox();

    QgsRubberBand* m_rubberBand;  // 临时显示的要素
    QgsFeature m_selectedFeature;  // 选中的要素
    QList<QgsFeature> m_movedFeatures; // 记录已经移动的特征
    QgsHighlight* m_highlight;

    QList<QgsMapLayer*> m_layers;
    QgsVectorLayer* m_layer;
    QgsMapLayer* m_curMapLayer;
    QgsVectorLayer* m_tempLayer = nullptr; // 定义临时图层的成员变量



    bool m_isSelecting = false;  // 是否处于选择模式
    bool isEditing = false;

    QgsFeature m_copiedFeature; // 用于保存复制的要素

    QSet<qint64> selectedFeatureIds; // 存储选择的要素ID
    QgsMapToolIdentifyFeature* identifyTool = nullptr; // 用于持久化工具实例

    QList<UndoAction> undoStack; // 撤销栈
    QList<UndoAction> redoStack; // 重做栈

    //符号库
    SymbolLibManager* symbolLibrary = nullptr;
    void mergeFeatures(const QList<QgsFeature>& features);
    QMap<QgsMapLayer*, QgsRectangle> m_layerExtents;//存储图层的初始大小
};
