#pragma once
#include <QListWidgetItem>
#include <QFileDialog>
#include <QIcon>
#include <QListWidget>
#include <QDialog>
#include <QDialog>
#include<qmessagebox.h>
#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QPixmap>
#include<qtablewidget.h>
#include<QPainter>
#include "ui_SymbolLibManager.h"
#include <QgsSymbol.h>
#include <QgsMapLayer.h>
#include <QXmlStreamReader>
#include <qgsstyle.h>
#include <qgsstylemanagerdialog.h>
#include <qgssymbol.h>
#include <qgsvectorlayer.h>
#include <QDir>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QListWidgetItem>
#include <QPixmap>
#include "StyleSheet.h"

class SymbolLibManager : public QDialog
{
	Q_OBJECT

public:
	SymbolLibManager(QWidget* parent = nullptr);
	~SymbolLibManager();

	void setCurrentLayer(QgsMapLayer* layer);//ѡ��ͼ��


private slots:

	void on_btnImport_clicked();      // ������ſ�
	void on_btnExportSLD_clicked();   // ����SLD�ļ�
	void on_applyButton_clicked();//������ʽӦ��
	void onLayerComboBoxIndexChanged(int index);//ѡ��ͼ��

private:
	Ui::SymbolLibManagerClass ui;

	StyleSheet style;
	void setStyle();

	QgsStyle* mStyle = nullptr;

	// ����XML���ſ�
	void importSymbolLibrary(const QString& fileName);
	//����SLD
	void importSLD(const QString& fileName);
	QgsMapLayer* m_curMapLayer; //��ǰѡ�е�ͼ��
	// �� SLD �ļ��� Rule Ԫ�ش�������
	QgsSymbol* createSymbolFromSLD(const QDomElement& ruleElement);
};
