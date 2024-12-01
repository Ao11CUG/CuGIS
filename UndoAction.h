#include <qgsfeature.h>
#include <qgsgeometry.h>

enum ActionType {
    ADD,
    MOVE,
    COPY,
    DELETE,
    LABEL
};

class UndoAction {
public:
    ActionType type;                     // ��������
    QList<QgsFeature> features;          // �����б�
    QgsPointXY centroidMidpoint;         // �ʵ�����
    QList<QgsGeometry> originalGeometries; // ԭʼ�������б�

    UndoAction(ActionType actionType)
        :type(actionType) {
    }
                                           
                                           // ʹ�� QgsFeature ����
    UndoAction(ActionType actionType, const QList<QgsFeature>& feats)
        : type(actionType) ,
        features(feats){
    }

    // ʹ�� QgsPointXY �� QgsFeature ����
    UndoAction(ActionType actionType, const QList<QgsFeature>& feats, const QgsPointXY& point)
        : type(actionType),
        centroidMidpoint(point),
        features(feats) {
    }

    // ʹ�� QgsGeometry ����
    UndoAction(ActionType actionType, const QList<QgsGeometry>& originalGeoms)
        : type(actionType) ,
        originalGeometries(originalGeoms){ // �������ڴ洢ԭʼ����
    }

    // ʹ�� QgsFeature �� QgsGeometry ͬʱ����
    UndoAction(ActionType actionType, const QList<QgsFeature>& feats, const QList<QgsGeometry>& originalGeoms)
        : type(actionType), features(feats), originalGeometries(originalGeoms) {}
};


