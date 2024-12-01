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

    Mode m_currentMode; // ��ǰ����ģʽ

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
    void on_extentLineAct_triggered();      //�ӳ���
    void on_rotateLineAct_triggered();  //��ת��
    void on_smoothGeometryAct_triggered();  //�⻬��
    void on_divideSurfaceAct_triggered();
    void on_simplifyLineAct_triggered();
    void on_extractBoundaryAct_triggered();
    void removeLayer();
    void on_setCrsAct_triggered();

    void on_addPointAct_triggered();//��ӵ�
    void on_selectFeatureAct_triggered();//ѡ��Ҫ��
    void on_copyFeatureAct_triggered();
    void on_moveFeatureAct_triggered();
    void on_startEditingAct_triggered();//��ʼ�༭
    void on_exitAct_triggered();
    void textLabel(const QgsPointXY& point, Qt::MouseButton button);

    void onLayerTreeItemDoubleClicked(const QModelIndex& index);//˫��ͼ�㺯��
    void on_viewAttributeAct_triggered();
    void on_symbolAct_triggered();
    void on_resetAct_triggered();

    //�ϲ�����غ���
    void on_faceMergeAct_triggered();
    void onCurrentLayerChanged(QgsMapLayer* layer);

    //ɾ����
    void on_deleteFeatureAct_triggered();

    void onCanvasReleased(const QgsPointXY& point, Qt::MouseButton button);//��ӵ㹤����Ӧ�ۺ���
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

    std::vector<QgsMapCanvasAnnotationItem*> m_annotations; // �洢ע����

    // ��ͼ����
    QgsMapCanvas* m_mapCanvas;

    //ӥ��ͼ
    EagleEye* m_eagleEyeWidget;
    void initEagleEye(QgsMapCanvas* mainCanvas);

    // ͼ�������
    void initLayerTreeView();
    QgsLayerTreeView* m_layerTreeView;
    QgsLayerTreeMapCanvasBridge* m_layerTreeCanvasBridge;

    // ������
    ToolBox* m_toolbox;
    void initToolBox();

    QgsRubberBand* m_rubberBand;  // ��ʱ��ʾ��Ҫ��
    QgsFeature m_selectedFeature;  // ѡ�е�Ҫ��
    QList<QgsFeature> m_movedFeatures; // ��¼�Ѿ��ƶ�������
    QgsHighlight* m_highlight;

    QList<QgsMapLayer*> m_layers;
    QgsVectorLayer* m_layer;
    QgsMapLayer* m_curMapLayer;
    QgsVectorLayer* m_tempLayer = nullptr; // ������ʱͼ��ĳ�Ա����



    bool m_isSelecting = false;  // �Ƿ���ѡ��ģʽ
    bool isEditing = false;

    QgsFeature m_copiedFeature; // ���ڱ��渴�Ƶ�Ҫ��

    QSet<qint64> selectedFeatureIds; // �洢ѡ���Ҫ��ID
    QgsMapToolIdentifyFeature* identifyTool = nullptr; // ���ڳ־û�����ʵ��

    QList<UndoAction> undoStack; // ����ջ
    QList<UndoAction> redoStack; // ����ջ

    //���ſ�
    SymbolLibManager* symbolLibrary = nullptr;
    void mergeFeatures(const QList<QgsFeature>& features);
    QMap<QgsMapLayer*, QgsRectangle> m_layerExtents;//�洢ͼ��ĳ�ʼ��С
};
