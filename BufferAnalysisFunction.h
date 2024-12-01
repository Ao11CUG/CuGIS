#pragma once
#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include "ui_BufferAnalysisFunction.h"
#include <qgsapplication.h>
#include <qgsvectorlayer.h>
#include <qgsfeature.h>
#include <qgsgeometry.h>
#include <qgsfield.h>
#include <qgsvectorfilewriter.h>
#include <qgsproject.h>                 
#include <qgscoordinatetransformcontext.h> 
#include "StyleSheet.h"
#include "qgsmapcanvas.h"

class BufferAnalysisFunction : public QMainWindow
{
	Q_OBJECT

public:
	BufferAnalysisFunction(QgsMapCanvas* m_mapCanvas, QWidget* parent);
	~BufferAnalysisFunction();

	QString strInputFilePath;  // 输入矢量文件路径
	QString strOutputFilePath; // 输出文件路径

private slots:
	void onChooseVectorFileClicked();  // 槽函数：选择矢量文件
	void onChooseSaveFileClicked();   // 槽函数：选择保存路径
	void onActionBufferAnalysisClicked(); // 槽函数：执行缓冲区分析
	void updateBox();

private:
	Ui::BufferAnalysisFunctionClass ui;
	QgsMapCanvas* m_mapCanvas;

	void setStyle();
	StyleSheet style;

	QList<QgsVectorLayer*> m_vectorLayers; // 存储图层的成员变量

	QgsVectorLayer* m_layer;

	void performBufferAnalysis(const QString& strInputFilePath, const QString& strOutputFilePath, double dBufferRadius);
};

