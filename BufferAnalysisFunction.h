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

	QString strInputFilePath;  // ����ʸ���ļ�·��
	QString strOutputFilePath; // ����ļ�·��

private slots:
	void onChooseVectorFileClicked();  // �ۺ�����ѡ��ʸ���ļ�
	void onChooseSaveFileClicked();   // �ۺ�����ѡ�񱣴�·��
	void onActionBufferAnalysisClicked(); // �ۺ�����ִ�л���������
	void updateBox();

private:
	Ui::BufferAnalysisFunctionClass ui;
	QgsMapCanvas* m_mapCanvas;

	void setStyle();
	StyleSheet style;

	QList<QgsVectorLayer*> m_vectorLayers; // �洢ͼ��ĳ�Ա����

	QgsVectorLayer* m_layer;

	void performBufferAnalysis(const QString& strInputFilePath, const QString& strOutputFilePath, double dBufferRadius);
};

