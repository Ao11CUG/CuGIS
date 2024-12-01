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
    ActionType type;                     // 操作类型
    QList<QgsFeature> features;          // 特征列表
    QgsPointXY centroidMidpoint;         // 质点中心
    QList<QgsGeometry> originalGeometries; // 原始几何体列表

    UndoAction(ActionType actionType)
        :type(actionType) {
    }
                                           
                                           // 使用 QgsFeature 构造
    UndoAction(ActionType actionType, const QList<QgsFeature>& feats)
        : type(actionType) ,
        features(feats){
    }

    // 使用 QgsPointXY 和 QgsFeature 构造
    UndoAction(ActionType actionType, const QList<QgsFeature>& feats, const QgsPointXY& point)
        : type(actionType),
        centroidMidpoint(point),
        features(feats) {
    }

    // 使用 QgsGeometry 构造
    UndoAction(ActionType actionType, const QList<QgsGeometry>& originalGeoms)
        : type(actionType) ,
        originalGeometries(originalGeoms){ // 可以用于存储原始几何
    }

    // 使用 QgsFeature 和 QgsGeometry 同时构造
    UndoAction(ActionType actionType, const QList<QgsFeature>& feats, const QList<QgsGeometry>& originalGeoms)
        : type(actionType), features(feats), originalGeometries(originalGeoms) {}
};


